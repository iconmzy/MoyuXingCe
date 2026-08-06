#pragma once

#include "application/controllers/MainController.h"

#include <QMainWindow>
#include <QPointer>

class QButtonGroup;
class QCloseEvent;
class QComboBox;
class QLabel;
class QFrame;
class QGraphicsOpacityEffect;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QStackedLayout;
class QTextBrowser;
class QTextEdit;
class QVBoxLayout;

namespace kgl::presentation {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(application::MainController* controller, QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void renderPapers(const QVector<domain::PaperSummary>& papers, int total);
    void renderPaper(const domain::PaperDetail& paper);
    void renderQuestion(const domain::OnlineQuestion& question, int index, int total);
    void renderAnswer(const domain::AnswerResult& result);
    void renderMode(application::MainController::AppMode mode);
    void setLoading(bool loading);
    void renderError(const QString& message);
    void loadSelectedPaper();
    void submitAnswer();
    void revealAnswer();
    void saveNotes();
    void jumpToQuestion();
    void toggleRealTitle();
    void showSettingsDialog();
    void renderPracticeProgress(int answered, int correct, int total);

private:
    QWidget* createPracticePage();
    QWidget* createOfficePage();
    void rebuildOptions(const QStringList& options);
    void resetQuestionState();
    void loadAppearanceSettings();
    void applyPracticeAppearance();
    void saveAppearanceSettings() const;

    application::MainController* controller_ = nullptr;
    QStackedLayout* pageStack_ = nullptr;
    QWidget* practicePage_ = nullptr;
    QWidget* officePage_ = nullptr;
    QFrame* practicePanel_ = nullptr;
    QGraphicsOpacityEffect* practiceOpacityEffect_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* accuracyLabel_ = nullptr;
    QLabel* paperTitleLabel_ = nullptr;
    QLabel* questionMetaLabel_ = nullptr;
    QLabel* resultLabel_ = nullptr;
    QComboBox* paperCombo_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* loadPaperButton_ = nullptr;
    QPushButton* previousButton_ = nullptr;
    QPushButton* nextButton_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QPushButton* revealButton_ = nullptr;
    QPushButton* realTitleButton_ = nullptr;
    QSpinBox* questionNumberSpin_ = nullptr;
    QButtonGroup* optionGroup_ = nullptr;
    QVBoxLayout* optionsLayout_ = nullptr;
    QTextBrowser* materialBrowser_ = nullptr;
    QTextBrowser* questionBrowser_ = nullptr;
    QTextBrowser* explanationBrowser_ = nullptr;
    QTextEdit* notesEdit_ = nullptr;
    QString realPaperTitle_;
    QString disguisedPaperTitle_;
    bool showingRealTitle_ = false;
    int currentQuestionTotal_ = 0;
    int practiceOpacityPercent_ = 88;
    int practiceFontSize_ = 15;
    int practicePanelWidth_ = 760;
    bool startInOfficeMode_ = true;
    QPointer<QObject> bufferedQTarget_;
    QString bufferedQText_;
    Qt::KeyboardModifiers bufferedQModifiers_ = Qt::NoModifier;
    bool qKeyHeld_ = false;
    bool qChordTriggered_ = false;
    bool suppressERelease_ = false;
    bool replayingBufferedKey_ = false;
};

} // namespace kgl::presentation
