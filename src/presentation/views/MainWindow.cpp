#include "presentation/views/MainWindow.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QFont>
#include <QFrame>
#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QStackedLayout>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString displayHtml(const QString& value, int fontSize = 15)
{
    if (value.trimmed().isEmpty()) {
        return {};
    }

    QString body = value;
    if (!(body.contains(QLatin1Char('<')) && body.contains(QLatin1Char('>')))) {
        body = Qt::convertFromPlainText(body);
    }
    body.replace(QStringLiteral("src=\"/"), QStringLiteral("src=\"https://balagk.com/"));
    body.replace(QStringLiteral("src='/"), QStringLiteral("src='https://balagk.com/"));
    return QStringLiteral(
        "<style>body{font-family:'Microsoft YaHei UI';font-size:%1px;line-height:1.65;"
        "color:#22272e;}p{margin:0 0 10px 0;}img{max-width:100%;height:auto;}</style>")
        .arg(fontSize)
        + body;
}

QString plainOptionText(const QString& value)
{
    if (!(value.contains(QLatin1Char('<')) && value.contains(QLatin1Char('>')))) {
        return value.trimmed();
    }
    QTextDocument document;
    document.setHtml(value);
    const QString text = document.toPlainText().trimmed();
    return text.isEmpty() ? value.trimmed() : text;
}

class OptionRow final : public QWidget {
public:
    OptionRow(const QString& label, const QString& text, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("optionRow"));
        setAttribute(Qt::WA_StyledBackground);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 6, 8, 6);
        layout->setAlignment(Qt::AlignTop);
        radioButton_ = new QRadioButton(label, this);
        radioButton_->setAccessibleName(label + QLatin1Char(' ') + text);
        auto* textLabel = new QLabel(text, this);
        textLabel->setWordWrap(true);
        textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(radioButton_);
        layout->addWidget(textLabel, 1);
        setCursor(Qt::PointingHandCursor);
    }

    QRadioButton* radioButton() const
    {
        return radioButton_;
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        radioButton_->setChecked(true);
        QWidget::mousePressEvent(event);
    }

private:
    QRadioButton* radioButton_ = nullptr;
};

} // namespace

namespace kgl::presentation {

MainWindow::MainWindow(application::MainController* controller, QWidget* parent)
    : QMainWindow(parent)
    , controller_(controller)
{
    setWindowTitle(QStringLiteral("项目工作台 - 工作记录"));
    resize(980, 720);

    auto* centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("workspaceRoot"));
    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* header = new QWidget(centralWidget);
    header->setObjectName(QStringLiteral("officeHeader"));
    auto* headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(18, 10, 14, 10);
    titleLabel_ = new QLabel(QStringLiteral("项目工作台"), header);
    QFont titleFont = titleLabel_->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    headerLayout->addWidget(titleLabel_);
    auto* sectionLabel = new QLabel(QStringLiteral("本周进度与工作记录"), header);
    sectionLabel->setObjectName(QStringLiteral("sectionLabel"));
    headerLayout->addWidget(sectionLabel);
    headerLayout->addStretch();
    auto* settingsButton = new QPushButton(QStringLiteral("设置"), header);
    settingsButton->setToolTip(QStringLiteral("调整审阅层外观和启动偏好"));
    headerLayout->addWidget(settingsButton);
    header->setLayout(headerLayout);
    rootLayout->addWidget(header);

    auto* workspace = new QWidget(centralWidget);
    pageStack_ = new QStackedLayout(workspace);
    pageStack_->setContentsMargins(0, 0, 0, 0);
    pageStack_->setStackingMode(QStackedLayout::StackAll);
    officePage_ = createOfficePage();
    practicePage_ = createPracticePage();
    pageStack_->addWidget(officePage_);
    pageStack_->addWidget(practicePage_);
    rootLayout->addWidget(workspace, 1);

    statusLabel_ = new QLabel(centralWidget);
    statusLabel_->setObjectName(QStringLiteral("statusBar"));
    statusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rootLayout->addWidget(statusLabel_);
    setCentralWidget(centralWidget);
    qApp->installEventFilter(this);
    loadAppearanceSettings();
    controller_->setMode(startInOfficeMode_
        ? application::MainController::AppMode::Office
        : application::MainController::AppMode::Practice);

    centralWidget->setStyleSheet(QStringLiteral(R"(
        QWidget#workspaceRoot { background: #edf1f4; color: #25313c; }
        QWidget#officeHeader { background: #ffffff; border-bottom: 1px solid #cfd6dc; }
        QLabel#sectionLabel { color: #6b747d; margin-left: 12px; }
        QLabel#statusBar { background: #f8fafb; color: #68727c; padding: 4px 14px; border-top: 1px solid #dce2e7; }
        QPushButton { min-height: 27px; padding: 2px 11px; border: 1px solid #b9c3cc; background: #f9fbfc; border-radius: 3px; }
        QPushButton:hover { background: #e8f0f5; border-color: #8296a6; }
        QPushButton:pressed { background: #dce8ee; }
        QPushButton:disabled { color: #9ca5ac; background: #eef1f3; border-color: #d9dee2; }
        QWidget#officePage { background: #f4f6f8; }
        QTableWidget#workTable { background: #ffffff; alternate-background-color: #f5f8fa; border: 1px solid #ccd5dc; gridline-color: #dce2e7; }
        QHeaderView::section { background: #e8edf1; color: #33414c; padding: 6px; border: none; border-right: 1px solid #ccd5dc; border-bottom: 1px solid #ccd5dc; }
        QTextEdit#documentEditor { background: #ffffff; border: 1px solid #ccd5dc; padding: 10px; selection-background-color: #8aa9bc; }
        QFrame#practicePanel { background-color: rgba(248, 250, 251, 218); border-left: 1px solid rgba(137, 151, 162, 170); }
        QFrame#practicePanel QLabel { background: transparent; }
        QTextBrowser#reviewText { background-color: rgba(255, 255, 255, 145); border: 1px solid rgba(176, 187, 195, 150); padding: 7px; }
        QWidget#optionRow { background-color: rgba(255, 255, 255, 100); border: 1px solid rgba(185, 195, 202, 120); border-radius: 2px; }
        QWidget#optionRow:hover { background-color: rgba(231, 240, 245, 185); border-color: rgba(113, 137, 152, 180); }
        QComboBox { min-height: 27px; background: rgba(255, 255, 255, 185); border: 1px solid rgba(166, 178, 187, 180); padding: 1px 7px; }
        QProgressBar { border: none; background: transparent; }
        QProgressBar::chunk { background: #64879a; }
    )"));

    connect(controller_, &application::MainController::papersChanged,
        this, &MainWindow::renderPapers);
    connect(controller_, &application::MainController::paperChanged,
        this, &MainWindow::renderPaper);
    connect(controller_, &application::MainController::questionChanged,
        this, &MainWindow::renderQuestion);
    connect(controller_, &application::MainController::answerEvaluated,
        this, &MainWindow::renderAnswer);
    connect(controller_, &application::MainController::loadingChanged,
        this, &MainWindow::setLoading);
    connect(controller_, &application::MainController::modeChanged,
        this, &MainWindow::renderMode);
    connect(controller_, &application::MainController::statusMessage,
        statusLabel_, &QLabel::setText);
    connect(controller_, &application::MainController::errorOccurred,
        this, &MainWindow::renderError);
    connect(settingsButton, &QPushButton::clicked,
        this, &MainWindow::showSettingsDialog);

    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("windowGeometry")).toByteArray());
    renderMode(controller_->mode());
    applyPracticeAppearance();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (replayingBufferedKey_) {
        return QObject::eventFilter(watched, event);
    }

    if (event->type() == QEvent::ApplicationDeactivate) {
        qKeyHeld_ = false;
        qChordTriggered_ = false;
        suppressERelease_ = false;
        bufferedQTarget_.clear();
        bufferedQText_.clear();
    }

    if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress
        || event->type() == QEvent::KeyRelease) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

        const bool ctrlSpace = keyEvent->key() == Qt::Key_Space
            && modifiers.testFlag(Qt::ControlModifier)
            && !modifiers.testFlag(Qt::AltModifier)
            && !modifiers.testFlag(Qt::ShiftModifier)
            && !modifiers.testFlag(Qt::MetaModifier);
        if (ctrlSpace) {
            keyEvent->accept();
            if (event->type() == QEvent::ShortcutOverride) {
                return true;
            }
            if (event->type() == QEvent::KeyPress && !keyEvent->isAutoRepeat()) {
                controller_->toggleMode();
            }
            return true;
        }

        const bool noModifiers = modifiers == Qt::NoModifier;
        if (keyEvent->key() == Qt::Key_Q && noModifiers) {
            keyEvent->accept();
            if (event->type() == QEvent::ShortcutOverride) {
                return true;
            }
            if (event->type() == QEvent::KeyPress) {
                if (!keyEvent->isAutoRepeat() && !qKeyHeld_) {
                    qKeyHeld_ = true;
                    bufferedQTarget_ = watched;
                    bufferedQText_ = keyEvent->text();
                    bufferedQModifiers_ = modifiers;
                }
                return true;
            }

            if (qKeyHeld_ && !qChordTriggered_ && bufferedQTarget_ != nullptr) {
                replayingBufferedKey_ = true;
                QKeyEvent replayPress(QEvent::KeyPress, Qt::Key_Q,
                    bufferedQModifiers_, bufferedQText_);
                QKeyEvent replayRelease(QEvent::KeyRelease, Qt::Key_Q,
                    bufferedQModifiers_, bufferedQText_);
                QApplication::sendEvent(bufferedQTarget_, &replayPress);
                QApplication::sendEvent(bufferedQTarget_, &replayRelease);
                replayingBufferedKey_ = false;
            }
            qKeyHeld_ = false;
            qChordTriggered_ = false;
            bufferedQTarget_.clear();
            bufferedQText_.clear();
            return true;
        }

        if (keyEvent->key() == Qt::Key_E && noModifiers && qKeyHeld_) {
            keyEvent->accept();
            if (event->type() == QEvent::ShortcutOverride) {
                return true;
            }
            if (event->type() == QEvent::KeyPress && !keyEvent->isAutoRepeat()) {
                qChordTriggered_ = true;
                suppressERelease_ = true;
                controller_->toggleMode();
            }
            return true;
        }

        if (keyEvent->key() == Qt::Key_E && event->type() == QEvent::KeyRelease
            && suppressERelease_) {
            suppressERelease_ = false;
            keyEvent->accept();
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

QWidget* MainWindow::createPracticePage()
{
    auto* page = new QWidget(this);
    page->setAttribute(Qt::WA_TranslucentBackground);
    auto* overlayLayout = new QHBoxLayout(page);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);
    overlayLayout->addStretch(1);

    practicePanel_ = new QFrame(page);
    practicePanel_->setObjectName(QStringLiteral("practicePanel"));
    practicePanel_->setAttribute(Qt::WA_StyledBackground);
    auto* layout = new QVBoxLayout(practicePanel_);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(8);
    overlayLayout->addWidget(practicePanel_, 4);

    auto* sourceLayout = new QHBoxLayout;
    auto* sourceLabel = new QLabel(QStringLiteral("数据集"), practicePanel_);
    paperCombo_ = new QComboBox(practicePanel_);
    paperCombo_->setMinimumWidth(400);
    paperCombo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    paperCombo_->setMinimumContentsLength(36);
    refreshButton_ = new QPushButton(QStringLiteral("同步"), practicePanel_);
    loadPaperButton_ = new QPushButton(QStringLiteral("打开"), practicePanel_);
    auto* sourceButton = new QPushButton(QStringLiteral("来源"), practicePanel_);
    realTitleButton_ = new QPushButton(QStringLiteral("显示标题"), practicePanel_);
    realTitleButton_->setToolTip(QStringLiteral("临时显示当前数据集的真实标题"));
    sourceLayout->addWidget(sourceLabel);
    sourceLayout->addWidget(paperCombo_, 1);
    sourceLayout->addWidget(refreshButton_);
    sourceLayout->addWidget(loadPaperButton_);
    sourceLayout->addWidget(sourceButton);
    sourceLayout->addWidget(realTitleButton_);
    layout->addLayout(sourceLayout);

    progressBar_ = new QProgressBar(practicePanel_);
    progressBar_->setRange(0, 0);
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(4);
    progressBar_->setVisible(false);
    layout->addWidget(progressBar_);

    paperTitleLabel_ = new QLabel(QStringLiteral("请选择一个数据集"), practicePanel_);
    QFont paperFont = paperTitleLabel_->font();
    paperFont.setPointSize(12);
    paperFont.setBold(true);
    paperTitleLabel_->setFont(paperFont);
    paperTitleLabel_->setWordWrap(true);
    layout->addWidget(paperTitleLabel_);

    questionMetaLabel_ = new QLabel(practicePanel_);
    questionMetaLabel_->setStyleSheet(QStringLiteral("color: #68707d;"));
    layout->addWidget(questionMetaLabel_);

    materialBrowser_ = new QTextBrowser(practicePanel_);
    materialBrowser_->setObjectName(QStringLiteral("reviewText"));
    materialBrowser_->setOpenExternalLinks(true);
    materialBrowser_->document()->setBaseUrl(QUrl(QStringLiteral("https://balagk.com/")));
    materialBrowser_->setMaximumHeight(150);
    materialBrowser_->setVisible(false);
    layout->addWidget(materialBrowser_);

    questionBrowser_ = new QTextBrowser(practicePanel_);
    questionBrowser_->setObjectName(QStringLiteral("reviewText"));
    questionBrowser_->setOpenExternalLinks(true);
    questionBrowser_->document()->setBaseUrl(QUrl(QStringLiteral("https://balagk.com/")));
    questionBrowser_->setMinimumHeight(130);
    layout->addWidget(questionBrowser_, 2);

    auto* optionsWidget = new QWidget(practicePanel_);
    optionsLayout_ = new QVBoxLayout(optionsWidget);
    optionsLayout_->setContentsMargins(0, 4, 0, 4);
    optionGroup_ = new QButtonGroup(this);
    optionGroup_->setExclusive(true);
    layout->addWidget(optionsWidget);

    resultLabel_ = new QLabel(practicePanel_);
    resultLabel_->setWordWrap(true);
    layout->addWidget(resultLabel_);

    explanationBrowser_ = new QTextBrowser(practicePanel_);
    explanationBrowser_->setObjectName(QStringLiteral("reviewText"));
    explanationBrowser_->setOpenExternalLinks(true);
    explanationBrowser_->document()->setBaseUrl(QUrl(QStringLiteral("https://balagk.com/")));
    explanationBrowser_->setMinimumHeight(130);
    explanationBrowser_->setVisible(false);
    layout->addWidget(explanationBrowser_, 2);

    auto* actionsLayout = new QHBoxLayout;
    auto* jumpLabel = new QLabel(QStringLiteral("定位"), practicePanel_);
    questionNumberSpin_ = new QSpinBox(practicePanel_);
    questionNumberSpin_->setRange(1, 1);
    questionNumberSpin_->setFixedWidth(58);
    questionNumberSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    auto* jumpButton = new QPushButton(QStringLiteral("跳转"), practicePanel_);
    actionsLayout->addWidget(jumpLabel);
    actionsLayout->addWidget(questionNumberSpin_);
    actionsLayout->addWidget(jumpButton);
    actionsLayout->addSpacing(12);
    previousButton_ = new QPushButton(QStringLiteral("上一条"), practicePanel_);
    submitButton_ = new QPushButton(QStringLiteral("确认"), practicePanel_);
    revealButton_ = new QPushButton(QStringLiteral("查看详情"), practicePanel_);
    nextButton_ = new QPushButton(QStringLiteral("下一条"), practicePanel_);
    actionsLayout->addWidget(previousButton_);
    actionsLayout->addWidget(submitButton_);
    actionsLayout->addWidget(revealButton_);
    actionsLayout->addWidget(nextButton_);
    actionsLayout->addStretch();
    layout->addLayout(actionsLayout);

    loadPaperButton_->setEnabled(false);
    realTitleButton_->setEnabled(false);
    questionNumberSpin_->setEnabled(false);
    previousButton_->setEnabled(false);
    nextButton_->setEnabled(false);
    submitButton_->setEnabled(false);
    revealButton_->setEnabled(false);

    connect(refreshButton_, &QPushButton::clicked,
        controller_, &application::MainController::refreshPapers);
    connect(loadPaperButton_, &QPushButton::clicked,
        this, &MainWindow::loadSelectedPaper);
    connect(sourceButton, &QPushButton::clicked,
        this, [this]() { QDesktopServices::openUrl(controller_->sourceUrl()); });
    connect(realTitleButton_, &QPushButton::clicked,
        this, &MainWindow::toggleRealTitle);
    connect(jumpButton, &QPushButton::clicked,
        this, &MainWindow::jumpToQuestion);
    connect(questionNumberSpin_, &QSpinBox::editingFinished,
        this, &MainWindow::jumpToQuestion);
    connect(previousButton_, &QPushButton::clicked,
        controller_, &application::MainController::previousQuestion);
    connect(nextButton_, &QPushButton::clicked,
        controller_, &application::MainController::nextQuestion);
    connect(submitButton_, &QPushButton::clicked,
        this, &MainWindow::submitAnswer);
    connect(revealButton_, &QPushButton::clicked,
        this, &MainWindow::revealAnswer);
    return page;
}

QWidget* MainWindow::createOfficePage()
{
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("officePage"));
    page->setAttribute(Qt::WA_StyledBackground);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(10);

    auto* headingLayout = new QHBoxLayout;
    auto* heading = new QLabel(QStringLiteral("项目进度跟踪"), page);
    QFont headingFont = heading->font();
    headingFont.setPointSize(13);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    headingLayout->addWidget(heading);
    headingLayout->addStretch();
    auto* dateLabel = new QLabel(QDate::currentDate().toString(QStringLiteral("yyyy年M月d日")), page);
    dateLabel->setStyleSheet(QStringLiteral("color: #697680;"));
    headingLayout->addWidget(dateLabel);
    layout->addLayout(headingLayout);

    auto* workTable = new QTableWidget(7, 5, page);
    workTable->setObjectName(QStringLiteral("workTable"));
    workTable->setHorizontalHeaderLabels({
        QStringLiteral("工作事项"),
        QStringLiteral("负责人"),
        QStringLiteral("优先级"),
        QStringLiteral("当前状态"),
        QStringLiteral("计划日期"),
    });
    const QList<QStringList> rows = {
        { QStringLiteral("需求与计划确认"), QStringLiteral("项目组"), QStringLiteral("高"), QStringLiteral("进行中"), QStringLiteral("本周") },
        { QStringLiteral("数据汇总与复核"), QStringLiteral("本人"), QStringLiteral("高"), QStringLiteral("待更新"), QStringLiteral("本周") },
        { QStringLiteral("阶段材料整理"), QStringLiteral("本人"), QStringLiteral("中"), QStringLiteral("进行中"), QStringLiteral("本周") },
        { QStringLiteral("会议事项跟进"), QStringLiteral("协作组"), QStringLiteral("中"), QStringLiteral("待确认"), QStringLiteral("下周") },
    };
    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows.at(row).size(); ++column) {
            workTable->setItem(row, column, new QTableWidgetItem(rows.at(row).at(column)));
        }
    }
    workTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < workTable->columnCount(); ++column) {
        workTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    workTable->verticalHeader()->setVisible(false);
    workTable->setAlternatingRowColors(true);
    workTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(workTable, 3);

    auto* notesHeading = new QLabel(QStringLiteral("会议与跟进记录"), page);
    QFont notesFont = notesHeading->font();
    notesFont.setBold(true);
    notesHeading->setFont(notesFont);
    layout->addWidget(notesHeading);

    notesEdit_ = new QTextEdit(page);
    notesEdit_->setObjectName(QStringLiteral("documentEditor"));
    notesEdit_->setPlaceholderText(QStringLiteral("记录会议结论、待办事项和后续计划"));
    layout->addWidget(notesEdit_, 2);

    auto* actionsLayout = new QHBoxLayout;
    auto* saveButton = new QPushButton(QStringLiteral("保存"), page);
    actionsLayout->addWidget(saveButton);
    actionsLayout->addStretch();
    layout->addLayout(actionsLayout);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveNotes);
    return page;
}

void MainWindow::renderPapers(const QVector<domain::PaperSummary>& papers, int total)
{
    paperCombo_->clear();
    for (int index = 0; index < papers.size(); ++index) {
        const domain::PaperSummary& paper = papers.at(index);
        const QString label = QStringLiteral("数据集 %1 · %2 条")
            .arg(index + 1, 2, 10, QLatin1Char('0'))
            .arg(paper.questionCount);
        paperCombo_->addItem(label, paper.id);
    }
    loadPaperButton_->setEnabled(!papers.isEmpty());
    paperCombo_->setToolTip(QStringLiteral("当前显示最新 %1 套，共 %2 套").arg(papers.size()).arg(total));
}

void MainWindow::renderPaper(const domain::PaperDetail& paper)
{
    realPaperTitle_ = paper.summary.title;
    disguisedPaperTitle_ = QStringLiteral("数据审阅记录 · %1 条").arg(paper.questions.size());
    currentQuestionTotal_ = paper.questions.size();
    showingRealTitle_ = false;
    realTitleButton_->setText(QStringLiteral("显示标题"));
    realTitleButton_->setEnabled(!realPaperTitle_.isEmpty());
    questionNumberSpin_->setRange(1, qMax(1, paper.questions.size()));
    questionNumberSpin_->setEnabled(!paper.questions.isEmpty());
    paperTitleLabel_->setText(disguisedPaperTitle_);
}

void MainWindow::renderQuestion(const domain::OnlineQuestion& question, int index, int total)
{
    const QStringList metadata = {
        QStringLiteral("第 %1 / %2 题").arg(index + 1).arg(total),
        question.knowledgePoint,
        question.subCategory,
        question.difficulty,
    };
    QStringList visibleMetadata;
    for (const QString& item : metadata) {
        if (!item.trimmed().isEmpty()) {
            visibleMetadata.append(item);
        }
    }
    questionMetaLabel_->setText(visibleMetadata.join(QStringLiteral(" · ")));
    questionNumberSpin_->setValue(index + 1);

    materialBrowser_->setVisible(!question.material.trimmed().isEmpty());
    materialBrowser_->setHtml(displayHtml(question.material, practiceFontSize_));
    questionBrowser_->setHtml(displayHtml(question.question, practiceFontSize_));
    rebuildOptions(question.options);
    previousButton_->setEnabled(index > 0);
    nextButton_->setEnabled(index + 1 < total);
    submitButton_->setEnabled(!question.options.isEmpty());
    revealButton_->setEnabled(
        !question.answer.trimmed().isEmpty() || !question.explanation.trimmed().isEmpty());
    resetQuestionState();
}

void MainWindow::renderAnswer(const domain::AnswerResult& result)
{
    if (!result.answered) {
        resultLabel_->setStyleSheet(QStringLiteral("color: #b54747;"));
        resultLabel_->setText(QStringLiteral("请选择一个答案"));
        return;
    }

    const bool hasSelection = !result.selectedAnswer.isEmpty();
    const bool hasAnswer = !result.correctAnswer.isEmpty();
    if (!hasAnswer) {
        resultLabel_->setStyleSheet(QStringLiteral("color: #68707d; font-weight: 600;"));
        resultLabel_->setText(hasSelection
            ? QStringLiteral("已选择 %1，本站暂未提供标准答案").arg(result.selectedAnswer)
            : QStringLiteral("本站暂未提供标准答案"));
    } else if (!hasSelection) {
        resultLabel_->setStyleSheet(QStringLiteral("color: #2e7d32; font-weight: 600;"));
        resultLabel_->setText(QStringLiteral("正确答案：%1").arg(result.correctAnswer));
    } else {
        resultLabel_->setStyleSheet(result.correct
            ? QStringLiteral("color: #2e7d32; font-weight: 600;")
            : QStringLiteral("color: #b54747; font-weight: 600;"));
        resultLabel_->setText(result.correct
            ? QStringLiteral("回答正确，正确答案：%1").arg(result.correctAnswer)
            : QStringLiteral("回答错误，正确答案：%1").arg(result.correctAnswer));
    }

    for (QAbstractButton* button : optionGroup_->buttons()) {
        const QString option = QString(QChar::fromLatin1(static_cast<char>('A' + optionGroup_->id(button))));
        if (option == result.correctAnswer) {
            button->parentWidget()->setStyleSheet(
                QStringLiteral("color: #2e7d32; font-weight: 600; background: #edf7ee;"));
        } else if (hasSelection && option == result.selectedAnswer && !result.correct) {
            button->parentWidget()->setStyleSheet(
                QStringLiteral("color: #b54747; font-weight: 600; background: #fff0f0;"));
        }
    }

    if (result.explanation.trimmed().isEmpty()) {
        explanationBrowser_->setHtml(displayHtml(QStringLiteral("本站暂未提供本题解析。"), practiceFontSize_));
    } else {
        explanationBrowser_->setHtml(displayHtml(result.explanation, practiceFontSize_));
    }
    explanationBrowser_->setVisible(true);
}

void MainWindow::renderMode(application::MainController::AppMode mode)
{
    const bool office = mode == application::MainController::AppMode::Office;
    if (!office && pageStack_->currentIndex() == 1) {
        saveNotes();
    }
    officePage_->setVisible(true);
    if (office) {
        pageStack_->setCurrentWidget(officePage_);
        practicePage_->setVisible(false);
    } else {
        practicePage_->setVisible(true);
        pageStack_->setCurrentWidget(practicePage_);
        practicePage_->raise();
    }
    titleLabel_->setText(QStringLiteral("项目工作台"));
    if (office) {
        notesEdit_->setPlainText(controller_->loadNotes());
    }
}

void MainWindow::setLoading(bool loading)
{
    progressBar_->setVisible(loading);
    refreshButton_->setEnabled(!loading);
    paperCombo_->setEnabled(!loading);
    loadPaperButton_->setEnabled(!loading && paperCombo_->count() > 0);
    if (loading) {
        realTitleButton_->setEnabled(false);
        questionNumberSpin_->setEnabled(false);
    } else if (!realPaperTitle_.isEmpty()) {
        realTitleButton_->setEnabled(true);
        questionNumberSpin_->setEnabled(currentQuestionTotal_ > 0);
    }
}

void MainWindow::renderError(const QString& message)
{
    resultLabel_->setStyleSheet(QStringLiteral("color: #b54747;"));
    resultLabel_->setText(message);
}

void MainWindow::loadSelectedPaper()
{
    const int paperId = paperCombo_->currentData().toInt();
    if (paperId > 0) {
        controller_->loadPaper(paperId);
    }
}

void MainWindow::submitAnswer()
{
    controller_->submitAnswer(optionGroup_->checkedId());
}

void MainWindow::revealAnswer()
{
    controller_->revealAnswer();
}

void MainWindow::saveNotes()
{
    controller_->saveNotes(notesEdit_->toPlainText());
}

void MainWindow::jumpToQuestion()
{
    const int number = questionNumberSpin_->value();
    if (number < 1 || number > currentQuestionTotal_
        || controller_->currentQuestion() == nullptr) {
        return;
    }
    controller_->goToQuestion(number);
}

void MainWindow::toggleRealTitle()
{
    showingRealTitle_ = !showingRealTitle_;
    realTitleButton_->setText(showingRealTitle_
        ? QStringLiteral("隐藏标题")
        : QStringLiteral("显示标题"));
    paperTitleLabel_->setText(showingRealTitle_
        ? realPaperTitle_
        : disguisedPaperTitle_);
}

void MainWindow::showSettingsDialog()
{
    const application::MainController::AppMode originalMode = controller_->mode();
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QStringLiteral("工作台设置"));
    dialog->setMinimumWidth(440);

    auto* root = new QVBoxLayout(dialog);
    auto* heading = new QLabel(QStringLiteral("审阅层外观"), dialog);
    QFont headingFont = heading->font();
    headingFont.setPointSize(12);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    root->addWidget(heading);

    auto* description = new QLabel(
        QStringLiteral("调整结果会即时预览并自动保存。透明度过低会影响阅读，建议保持在 65% 以上。"), dialog);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #65717a;"));
    root->addWidget(description);

    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto* opacityRow = new QWidget(dialog);
    auto* opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    auto* opacitySlider = new QSlider(Qt::Horizontal, opacityRow);
    opacitySlider->setRange(45, 100);
    opacitySlider->setValue(practiceOpacityPercent_);
    auto* opacityValue = new QLabel(QStringLiteral("%1%").arg(practiceOpacityPercent_), opacityRow);
    opacityValue->setFixedWidth(42);
    opacityValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    opacityLayout->addWidget(opacitySlider, 1);
    opacityLayout->addWidget(opacityValue);
    form->addRow(QStringLiteral("整体透明度"), opacityRow);

    auto* fontSpin = new QSpinBox(dialog);
    fontSpin->setRange(12, 22);
    fontSpin->setSuffix(QStringLiteral(" px"));
    fontSpin->setValue(practiceFontSize_);
    form->addRow(QStringLiteral("内容字体"), fontSpin);

    auto* widthSlider = new QSlider(Qt::Horizontal, dialog);
    widthSlider->setRange(620, 920);
    widthSlider->setSingleStep(20);
    widthSlider->setPageStep(40);
    widthSlider->setValue(practicePanelWidth_);
    form->addRow(QStringLiteral("面板宽度"), widthSlider);

    auto* startOfficeCheck = new QCheckBox(QStringLiteral("启动时先显示办公工作台"), dialog);
    startOfficeCheck->setChecked(startInOfficeMode_);
    form->addRow(QString(), startOfficeCheck);
    root->addLayout(form);

    auto* actions = new QHBoxLayout;
    auto* previewButton = new QPushButton(QStringLiteral("预览审阅层"), dialog);
    actions->addWidget(previewButton);
    actions->addStretch();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("完成"));
    actions->addWidget(buttons);
    root->addLayout(actions);

    connect(opacitySlider, &QSlider::valueChanged, dialog,
        [this, opacityValue](int value) {
            practiceOpacityPercent_ = value;
            opacityValue->setText(QStringLiteral("%1%").arg(value));
            applyPracticeAppearance();
            saveAppearanceSettings();
        });
    connect(fontSpin, &QSpinBox::valueChanged, dialog,
        [this](int value) {
            practiceFontSize_ = value;
            applyPracticeAppearance();
            saveAppearanceSettings();
        });
    connect(widthSlider, &QSlider::valueChanged, dialog,
        [this](int value) {
            practicePanelWidth_ = value;
            applyPracticeAppearance();
            saveAppearanceSettings();
        });
    connect(startOfficeCheck, &QCheckBox::toggled, dialog,
        [this](bool checked) {
            startInOfficeMode_ = checked;
            saveAppearanceSettings();
        });
    connect(previewButton, &QPushButton::clicked, dialog,
        [this, previewButton]() {
            const bool showingPreview = controller_->mode()
                == application::MainController::AppMode::Practice;
            controller_->setMode(showingPreview
                    ? application::MainController::AppMode::Office
                    : application::MainController::AppMode::Practice);
            previewButton->setText(showingPreview
                    ? QStringLiteral("预览审阅层")
                    : QStringLiteral("返回工作台"));
        });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(dialog, &QDialog::finished, this,
        [this, originalMode](int) { controller_->setMode(originalMode); });
    dialog->open();
}

void MainWindow::loadAppearanceSettings()
{
    QSettings settings;
    practiceOpacityPercent_ = qBound(45,
        settings.value(QStringLiteral("appearance/practiceOpacity"), 88).toInt(), 100);
    practiceFontSize_ = qBound(12,
        settings.value(QStringLiteral("appearance/practiceFontSize"), 15).toInt(), 22);
    practicePanelWidth_ = qBound(620,
        settings.value(QStringLiteral("appearance/practicePanelWidth"), 760).toInt(), 920);
    startInOfficeMode_ = settings.value(
        QStringLiteral("behavior/startInOfficeMode"), true).toBool();
}

void MainWindow::applyPracticeAppearance()
{
    if (practicePanel_ == nullptr) {
        return;
    }
    practicePanel_->setMinimumWidth(practicePanelWidth_);
    practicePanel_->setMaximumWidth(practicePanelWidth_);
    if (practiceOpacityEffect_ == nullptr) {
        practiceOpacityEffect_ = new QGraphicsOpacityEffect(practicePanel_);
        practicePanel_->setGraphicsEffect(practiceOpacityEffect_);
    }
    practiceOpacityEffect_->setOpacity(practiceOpacityPercent_ / 100.0);

    QFont contentFont = practicePanel_->font();
    contentFont.setPixelSize(practiceFontSize_);
    questionBrowser_->setFont(contentFont);
    materialBrowser_->setFont(contentFont);
    explanationBrowser_->setFont(contentFont);
    for (QAbstractButton* button : optionGroup_->buttons()) {
        button->parentWidget()->setFont(contentFont);
    }
}

void MainWindow::saveAppearanceSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/practiceOpacity"), practiceOpacityPercent_);
    settings.setValue(QStringLiteral("appearance/practiceFontSize"), practiceFontSize_);
    settings.setValue(QStringLiteral("appearance/practicePanelWidth"), practicePanelWidth_);
    settings.setValue(QStringLiteral("behavior/startInOfficeMode"), startInOfficeMode_);
}

void MainWindow::rebuildOptions(const QStringList& options)
{
    for (QAbstractButton* button : optionGroup_->buttons()) {
        optionGroup_->removeButton(button);
    }
    while (QLayoutItem* item = optionsLayout_->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    for (int i = 0; i < options.size(); ++i) {
        auto* row = new OptionRow(
            QStringLiteral("%1.").arg(QChar::fromLatin1(static_cast<char>('A' + i))),
            plainOptionText(options.at(i)));
        optionGroup_->addButton(row->radioButton(), i);
        optionsLayout_->addWidget(row);
    }
}

void MainWindow::resetQuestionState()
{
    optionGroup_->setExclusive(false);
    for (QAbstractButton* button : optionGroup_->buttons()) {
        button->setChecked(false);
        button->parentWidget()->setStyleSheet(QString());
    }
    optionGroup_->setExclusive(true);
    resultLabel_->clear();
    explanationBrowser_->clear();
    explanationBrowser_->setVisible(false);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("windowGeometry"), saveGeometry());
    if (controller_->mode() == application::MainController::AppMode::Office) {
        saveNotes();
    }
    QMainWindow::closeEvent(event);
}

} // namespace kgl::presentation
