#pragma once

#include "domain/entities/OnlineQuestion.h"

#include <QObject>
#include <QUrl>

#include <memory>

namespace kgl::domain {
class INotesRepository;
}

namespace kgl::infrastructure {
class BalaApiClient;
}

namespace kgl::application {

class MainController final : public QObject {
    Q_OBJECT

public:
    enum class AppMode {
        Practice,
        Office,
    };
    Q_ENUM(AppMode)

    explicit MainController(QObject* parent = nullptr);
    ~MainController() override;

    bool initialize();
    QUrl sourceUrl() const;
    AppMode mode() const;
    const domain::OnlineQuestion* currentQuestion() const;

    void refreshPapers();
    void loadPaper(int paperId);
    void submitAnswer(int optionIndex);
    void revealAnswer();
    void previousQuestion();
    void nextQuestion();
    void goToQuestion(int number);

    void setMode(AppMode mode);
    void toggleMode();

    QString loadNotes();
    bool saveNotes(const QString& content);

signals:
    void papersChanged(const QVector<domain::PaperSummary>& papers, int total);
    void paperChanged(const domain::PaperDetail& paper);
    void questionChanged(const domain::OnlineQuestion& question, int index, int total);
    void answerEvaluated(const domain::AnswerResult& result);
    void loadingChanged(bool loading);
    void modeChanged(kgl::application::MainController::AppMode mode);
    void statusMessage(const QString& message);
    void errorOccurred(const QString& message);

private:
    void publishCurrentQuestion();

    std::unique_ptr<infrastructure::BalaApiClient> apiClient_;
    std::unique_ptr<domain::INotesRepository> notesRepository_;
    domain::PaperDetail currentPaper_;
    int currentQuestionIndex_ = -1;
    // Start in the innocuous work view; Ctrl+Space reveals the review panel.
    AppMode mode_ = AppMode::Office;
};

} // namespace kgl::application

Q_DECLARE_METATYPE(kgl::application::MainController::AppMode)
