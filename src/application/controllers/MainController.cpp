#include "application/controllers/MainController.h"

#include "domain/repositories/INotesRepository.h"
#include "infrastructure/network/BalaApiClient.h"
#include "infrastructure/storage/JsonNotesRepository.h"

#include <QSettings>

namespace kgl::application {

MainController::MainController(QObject* parent)
    : QObject(parent)
    , apiClient_(std::make_unique<infrastructure::BalaApiClient>())
    , notesRepository_(std::make_unique<infrastructure::JsonNotesRepository>())
{
    connect(apiClient_.get(), &infrastructure::BalaApiClient::papersFetched,
        this, [this](const QVector<domain::PaperSummary>& papers, int total) {
            emit loadingChanged(false);
            emit papersChanged(papers, total);
            emit statusMessage(QStringLiteral("已同步 %1 / %2 个数据集").arg(papers.size()).arg(total));
        });
    connect(apiClient_.get(), &infrastructure::BalaApiClient::paperFetched,
        this, [this](const domain::PaperDetail& paper) {
            currentPaper_ = paper;
            loadPracticeProgress();
            emit loadingChanged(false);
            emit paperChanged(currentPaper_);
            publishCurrentQuestion();
            publishPracticeProgress();
            emit statusMessage(QStringLiteral("数据已打开，共 %1 条记录").arg(currentPaper_.questions.size()));
        });
    connect(apiClient_.get(), &infrastructure::BalaApiClient::requestFailed,
        this, [this](const QString& message) {
            emit loadingChanged(false);
            emit errorOccurred(message);
            emit statusMessage(message);
        });
}

MainController::~MainController() = default;

bool MainController::initialize()
{
    refreshPapers();
    return true;
}

QUrl MainController::sourceUrl() const
{
    return QUrl(QStringLiteral("https://balagk.com/%E9%A2%98%E5%BA%93/"));
}

MainController::AppMode MainController::mode() const
{
    return mode_;
}

const domain::OnlineQuestion* MainController::currentQuestion() const
{
    if (currentQuestionIndex_ < 0 || currentQuestionIndex_ >= currentPaper_.questions.size()) {
        return nullptr;
    }
    return &currentPaper_.questions.at(currentQuestionIndex_);
}

void MainController::refreshPapers()
{
    emit loadingChanged(true);
    emit statusMessage(QStringLiteral("正在同步云端数据…"));
    apiClient_->fetchPapers(1, 50);
}

void MainController::loadPaper(int paperId)
{
    emit loadingChanged(true);
    emit statusMessage(QStringLiteral("正在建立只读会话并读取数据…"));
    apiClient_->fetchPaper(paperId);
}

void MainController::submitAnswer(int optionIndex)
{
    const domain::OnlineQuestion* question = currentQuestion();
    domain::AnswerResult result;
    if (question == nullptr || optionIndex < 0 || optionIndex >= question->options.size()) {
        emit answerEvaluated(result);
        return;
    }

    result.answered = true;
    result.selectedAnswer = QString(QChar::fromLatin1(static_cast<char>('A' + optionIndex)));
    result.correctAnswer = question->answer.trimmed().toUpper();
    result.correct = result.selectedAnswer == result.correctAnswer;
    result.explanation = question->explanation;
    if (currentQuestionIndex_ >= 0 && currentQuestionIndex_ < answerResults_.size()) {
        answerResults_[currentQuestionIndex_] = result;
        answerRecorded_[currentQuestionIndex_] = true;
        savePracticeProgress();
        publishPracticeProgress();
    }
    emit answerEvaluated(result);
}

void MainController::revealAnswer()
{
    const domain::OnlineQuestion* question = currentQuestion();
    if (question == nullptr) {
        return;
    }

    domain::AnswerResult result;
    result.answered = true;
    result.correctAnswer = question->answer.trimmed().toUpper();
    result.explanation = question->explanation;
    emit answerEvaluated(result);
}

void MainController::previousQuestion()
{
    if (currentQuestionIndex_ <= 0) {
        return;
    }
    --currentQuestionIndex_;
    savePracticeProgress();
    publishCurrentQuestion();
}

void MainController::nextQuestion()
{
    if (currentQuestionIndex_ < 0 || currentQuestionIndex_ + 1 >= currentPaper_.questions.size()) {
        return;
    }
    ++currentQuestionIndex_;
    savePracticeProgress();
    publishCurrentQuestion();
}

void MainController::goToQuestion(int number)
{
    if (number < 1 || number > currentPaper_.questions.size()) {
        return;
    }
    currentQuestionIndex_ = number - 1;
    savePracticeProgress();
    publishCurrentQuestion();
}

void MainController::publishCurrentQuestion()
{
    const domain::OnlineQuestion* question = currentQuestion();
    if (question == nullptr) {
        return;
    }
    emit questionChanged(*question, currentQuestionIndex_, currentPaper_.questions.size());
    if (currentQuestionIndex_ < answerRecorded_.size()
        && answerRecorded_.at(currentQuestionIndex_)) {
        emit answerEvaluated(answerResults_.at(currentQuestionIndex_));
    }
}

void MainController::loadPracticeProgress()
{
    const int questionCount = currentPaper_.questions.size();
    answerResults_ = QVector<domain::AnswerResult>(questionCount);
    answerRecorded_ = QVector<bool>(questionCount, false);
    currentQuestionIndex_ = questionCount == 0 ? -1 : 0;
    if (questionCount == 0 || currentPaper_.summary.id <= 0) {
        return;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("practiceProgress"));
    settings.beginGroup(QString::number(currentPaper_.summary.id));
    const QStringList selectedAnswers = settings.value(
        QStringLiteral("selectedAnswers")).toStringList();
    currentQuestionIndex_ = qBound(0,
        settings.value(QStringLiteral("lastQuestionIndex"), 0).toInt(), questionCount - 1);
    settings.endGroup();
    settings.endGroup();

    const int savedCount = qMin(questionCount, selectedAnswers.size());
    for (int index = 0; index < savedCount; ++index) {
        const QString selected = selectedAnswers.at(index).trimmed().toUpper();
        if (selected.isEmpty()) {
            continue;
        }
        const domain::OnlineQuestion& question = currentPaper_.questions.at(index);
        domain::AnswerResult result;
        result.answered = true;
        result.selectedAnswer = selected;
        result.correctAnswer = question.answer.trimmed().toUpper();
        result.correct = result.selectedAnswer == result.correctAnswer;
        result.explanation = question.explanation;
        answerResults_[index] = result;
        answerRecorded_[index] = true;
    }
}

void MainController::savePracticeProgress() const
{
    if (currentPaper_.summary.id <= 0 || answerRecorded_.isEmpty()) {
        return;
    }

    QStringList selectedAnswers;
    selectedAnswers.reserve(answerRecorded_.size());
    for (int index = 0; index < answerRecorded_.size(); ++index) {
        selectedAnswers.append(answerRecorded_.at(index)
            ? answerResults_.at(index).selectedAnswer
            : QString());
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("practiceProgress"));
    settings.beginGroup(QString::number(currentPaper_.summary.id));
    settings.setValue(QStringLiteral("selectedAnswers"), selectedAnswers);
    settings.setValue(QStringLiteral("lastQuestionIndex"), currentQuestionIndex_);
    settings.endGroup();
    settings.endGroup();
}

void MainController::publishPracticeProgress()
{
    int answered = 0;
    int correct = 0;
    for (int index = 0; index < answerRecorded_.size(); ++index) {
        if (!answerRecorded_.at(index)) {
            continue;
        }
        ++answered;
        if (answerResults_.at(index).correct) {
            ++correct;
        }
    }
    emit practiceProgressChanged(answered, correct, currentPaper_.questions.size());
}

void MainController::setMode(AppMode mode)
{
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    emit modeChanged(mode_);
}

void MainController::toggleMode()
{
    setMode(mode_ == AppMode::Practice ? AppMode::Office : AppMode::Practice);
}

QString MainController::loadNotes()
{
    QString error;
    const QString content = notesRepository_->load(&error);
    if (!error.isEmpty()) {
        emit statusMessage(QStringLiteral("备忘录读取失败：%1").arg(error));
    }
    return content;
}

bool MainController::saveNotes(const QString& content)
{
    QString error;
    if (!notesRepository_->save(content, &error)) {
        emit statusMessage(QStringLiteral("备忘录保存失败：%1").arg(error));
        return false;
    }
    emit statusMessage(QStringLiteral("备忘录已保存"));
    return true;
}

} // namespace kgl::application
