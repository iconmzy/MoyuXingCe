#include "application/controllers/MainController.h"

#include "domain/repositories/INotesRepository.h"
#include "infrastructure/network/BalaApiClient.h"
#include "infrastructure/storage/JsonNotesRepository.h"

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
            currentQuestionIndex_ = currentPaper_.questions.isEmpty() ? -1 : 0;
            emit loadingChanged(false);
            emit paperChanged(currentPaper_);
            publishCurrentQuestion();
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
    publishCurrentQuestion();
}

void MainController::nextQuestion()
{
    if (currentQuestionIndex_ < 0 || currentQuestionIndex_ + 1 >= currentPaper_.questions.size()) {
        return;
    }
    ++currentQuestionIndex_;
    publishCurrentQuestion();
}

void MainController::goToQuestion(int number)
{
    if (number < 1 || number > currentPaper_.questions.size()) {
        return;
    }
    currentQuestionIndex_ = number - 1;
    publishCurrentQuestion();
}

void MainController::publishCurrentQuestion()
{
    const domain::OnlineQuestion* question = currentQuestion();
    if (question == nullptr) {
        return;
    }
    emit questionChanged(*question, currentQuestionIndex_, currentPaper_.questions.size());
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
