#include "views/FocusPage.h"

#include "repositories/FocusRepository.h"
#include "repositories/SettingsRepository.h"
#include "repositories/TaskRepository.h"
#include "services/NotificationSoundPlayer.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

FocusPage::FocusPage(QWidget *parent)
    : QWidget(parent),
      timer_(this),
      soundPlayer_(new NotificationSoundPlayer(this))
{
    buildInterface();
    reloadSettings();
    refreshTasks();

    connect(&timer_, &FocusTimer::timeChanged,
            this, &FocusPage::updateTime);
    connect(&timer_, &FocusTimer::stateChanged,
            this, &FocusPage::updateState);
    connect(&timer_, &FocusTimer::sessionEnded,
            this, &FocusPage::handleSessionEnded);
}

void FocusPage::buildInterface()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(18);

    auto *timerCard = new QFrame(this);
    timerCard->setObjectName(QStringLiteral("card"));
    auto *timerLayout = new QVBoxLayout(timerCard);
    timerLayout->setContentsMargins(36, 30, 36, 30);
    timerLayout->setSpacing(16);

    phaseLabel_ = new QLabel(QStringLiteral("专注"), timerCard);
    phaseLabel_->setAlignment(Qt::AlignCenter);
    phaseLabel_->setObjectName(QStringLiteral("cardTitle"));

    timerLabel_ = new QLabel(QStringLiteral("25:00"), timerCard);
    timerLabel_->setAlignment(Qt::AlignCenter);
    timerLabel_->setObjectName(QStringLiteral("timerLabel"));

    progress_ = new QProgressBar(timerCard);
    progress_->setTextVisible(false);
    progress_->setRange(0, 25 * 60);
    progress_->setValue(0);
    progress_->setFixedHeight(10);

    cycleLabel_ = new QLabel(QStringLiteral("当前周期：0 / 4"), timerCard);
    cycleLabel_->setAlignment(Qt::AlignCenter);
    cycleLabel_->setObjectName(QStringLiteral("mutedLabel"));

    statusLabel_ = new QLabel(QStringLiteral("准备开始专注"), timerCard);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setObjectName(QStringLiteral("mutedLabel"));
    statusLabel_->setWordWrap(true);

    auto *controls = new QHBoxLayout;
    startButton_ = new QPushButton(QStringLiteral("开始"), timerCard);
    startButton_->setObjectName(QStringLiteral("primaryButton"));
    pauseButton_ = new QPushButton(QStringLiteral("暂停"), timerCard);
    stopButton_ = new QPushButton(QStringLiteral("提前结束"), timerCard);
    stopButton_->setObjectName(QStringLiteral("dangerButton"));
    pauseButton_->setEnabled(false);
    stopButton_->setEnabled(false);
    controls->addStretch();
    controls->addWidget(startButton_);
    controls->addWidget(pauseButton_);
    controls->addWidget(stopButton_);
    controls->addStretch();

    timerLayout->addWidget(phaseLabel_);
    timerLayout->addStretch();
    timerLayout->addWidget(timerLabel_);
    timerLayout->addWidget(progress_);
    timerLayout->addWidget(cycleLabel_);
    timerLayout->addWidget(statusLabel_);
    timerLayout->addStretch();
    timerLayout->addLayout(controls);

    auto *optionsCard = new QFrame(this);
    optionsCard->setObjectName(QStringLiteral("card"));
    optionsCard->setFixedWidth(330);
    auto *optionsLayout = new QVBoxLayout(optionsCard);
    optionsLayout->setContentsMargins(26, 26, 26, 26);
    optionsLayout->setSpacing(12);

    auto *optionsTitle = new QLabel(QStringLiteral("本次专注"), optionsCard);
    optionsTitle->setObjectName(QStringLiteral("cardTitle"));
    auto *taskLabel = new QLabel(QStringLiteral("关联任务"), optionsCard);
    taskCombo_ = new QComboBox(optionsCard);
    auto *phaseSelectLabel = new QLabel(QStringLiteral("计时类型"), optionsCard);
    phaseCombo_ = new QComboBox(optionsCard);
    phaseCombo_->addItem(QStringLiteral("专注"),
                         static_cast<int>(FocusTimer::Phase::Focus));
    phaseCombo_->addItem(QStringLiteral("短休息"),
                         static_cast<int>(FocusTimer::Phase::ShortBreak));
    phaseCombo_->addItem(QStringLiteral("长休息"),
                         static_cast<int>(FocusTimer::Phase::LongBreak));

    auto *tip = new QLabel(
        QStringLiteral("计时使用系统时间校正。最小化窗口或短暂卡顿后，"
                       "剩余时间仍会保持准确。"),
        optionsCard);
    tip->setObjectName(QStringLiteral("mutedLabel"));
    tip->setWordWrap(true);

    optionsLayout->addWidget(optionsTitle);
    optionsLayout->addSpacing(8);
    optionsLayout->addWidget(taskLabel);
    optionsLayout->addWidget(taskCombo_);
    optionsLayout->addWidget(phaseSelectLabel);
    optionsLayout->addWidget(phaseCombo_);
    optionsLayout->addSpacing(10);
    optionsLayout->addWidget(tip);
    optionsLayout->addStretch();

    root->addWidget(timerCard, 1);
    root->addWidget(optionsCard);

    connect(startButton_, &QPushButton::clicked,
            this, &FocusPage::startCurrentPhase);
    connect(pauseButton_, &QPushButton::clicked,
            this, &FocusPage::togglePause);
    connect(stopButton_, &QPushButton::clicked,
            this, &FocusPage::stopEarly);
    connect(phaseCombo_, &QComboBox::currentIndexChanged,
            this, &FocusPage::updateIdleDuration);
}

void FocusPage::reloadSettings()
{
    settings_ = SettingsRepository().loadTimerSettings();
    cycleLabel_->setText(QStringLiteral("当前周期：%1 / %2")
                             .arg(completedFocusCycles_ % settings_.cyclesBeforeLongBreak)
                             .arg(settings_.cyclesBeforeLongBreak));
    updateIdleDuration();
}

void FocusPage::refreshTasks()
{
    const int previousId = taskCombo_->currentData().toInt();
    taskCombo_->clear();
    taskCombo_->addItem(QStringLiteral("无关联任务"), -1);
    const auto tasks = TaskRepository().findAll(TaskRepository::Filter::All);
    for (const auto &task : tasks) {
        if (task.status != QStringLiteral("completed")
            && task.status != QStringLiteral("cancelled")) {
            taskCombo_->addItem(task.title, task.id);
        }
    }
    const int previousIndex = taskCombo_->findData(previousId);
    taskCombo_->setCurrentIndex(previousIndex >= 0 ? previousIndex : 0);
}

void FocusPage::selectTask(int taskId)
{
    if (timer_.state() != FocusTimer::State::Idle) {
        return;
    }
    refreshTasks();
    const int taskIndex = taskCombo_->findData(taskId);
    if (taskIndex >= 0) {
        taskCombo_->setCurrentIndex(taskIndex);
        selectPhase(FocusTimer::Phase::Focus);
    }
}

void FocusPage::startCurrentPhase()
{
    if (timer_.state() != FocusTimer::State::Idle) {
        return;
    }
    const auto phase = selectedPhase();
    currentTaskId_ = phase == FocusTimer::Phase::Focus
        ? taskCombo_->currentData().toInt()
        : currentTaskId_;
    timer_.start(phase, durationSeconds(phase), currentTaskId_);
}

void FocusPage::togglePause()
{
    if (timer_.state() == FocusTimer::State::Running) {
        timer_.pause();
    } else if (timer_.state() == FocusTimer::State::Paused) {
        timer_.resume();
    }
}

void FocusPage::stopEarly()
{
    completeTaskWhenSessionEnds_ = false;
    const bool hasLinkedTask = timer_.phase() == FocusTimer::Phase::Focus
        && currentTaskId_ > 0;

    if (hasLinkedTask) {
        QMessageBox dialog(QMessageBox::Question,
                           QStringLiteral("提前结束"),
                           QStringLiteral("实际用时会保存并计入专注统计。\n"
                                          "是否同时将关联任务标记为完成？"),
                           QMessageBox::NoButton,
                           this);
        auto *finishOnlyButton = dialog.addButton(
            QStringLiteral("仅结束计时"), QMessageBox::AcceptRole);
        auto *completeTaskButton = dialog.addButton(
            QStringLiteral("结束并完成任务"), QMessageBox::ActionRole);
        auto *cancelButton = dialog.addButton(
            QStringLiteral("取消"), QMessageBox::RejectRole);
        dialog.setDefaultButton(cancelButton);
        dialog.exec();

        if (dialog.clickedButton() == cancelButton) {
            return;
        }
        completeTaskWhenSessionEnds_ =
            dialog.clickedButton() == completeTaskButton;
        Q_UNUSED(finishOnlyButton);
    } else {
        const auto choice = QMessageBox::question(
            this,
            QStringLiteral("提前结束"),
            QStringLiteral("要结束当前计时并保存实际用时吗？\n"
                           "已产生的专注时间会计入统计。"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return;
        }
    }

    timer_.stopEarly();
}

void FocusPage::updateTime(int remainingSeconds, int plannedSeconds)
{
    timerLabel_->setText(formatSeconds(remainingSeconds));
    progress_->setRange(0, qMax(1, plannedSeconds));
    progress_->setValue(qMax(0, plannedSeconds - remainingSeconds));
}

void FocusPage::updateState(FocusTimer::State state)
{
    const bool idle = state == FocusTimer::State::Idle;
    startButton_->setEnabled(idle);
    pauseButton_->setEnabled(!idle);
    stopButton_->setEnabled(!idle);
    phaseCombo_->setEnabled(idle);
    taskCombo_->setEnabled(idle);

    if (state == FocusTimer::State::Running) {
        pauseButton_->setText(QStringLiteral("暂停"));
        phaseLabel_->setText(phaseText(timer_.phase()));
        statusLabel_->setText(QStringLiteral("正在%1……").arg(phaseText(timer_.phase())));
    } else if (state == FocusTimer::State::Paused) {
        pauseButton_->setText(QStringLiteral("继续"));
        statusLabel_->setText(QStringLiteral("计时已暂停"));
    } else {
        pauseButton_->setText(QStringLiteral("暂停"));
    }
}

void FocusPage::handleSessionEnded(FocusTimer::Phase phase,
                                   bool completed,
                                   int taskId,
                                   QDateTime startedAt,
                                   QDateTime endedAt,
                                   int plannedSeconds,
                                   int actualSeconds)
{
    QString error;
    const bool saved = FocusRepository().recordSession(
        taskId,
        phase,
        completed,
        startedAt,
        endedAt,
        plannedSeconds,
        actualSeconds,
        completed ? QString() : QStringLiteral("用户提前结束"),
        &error);
    if (!saved) {
        completeTaskWhenSessionEnds_ = false;
        statusLabel_->setText(QStringLiteral("专注记录保存失败：%1").arg(error));
        updateIdleDuration();
        return;
    }

    bool taskMarkedCompleted = false;
    if (phase == FocusTimer::Phase::Focus
        && taskId > 0
        && completeTaskWhenSessionEnds_) {
        QString taskError;
        taskMarkedCompleted =
            TaskRepository().setCompleted(taskId, true, &taskError);
        if (!taskMarkedCompleted) {
            QMessageBox::warning(
                this,
                QStringLiteral("任务状态更新失败"),
                QStringLiteral("专注记录已经保存，但关联任务未能标记为完成：\n%1")
                    .arg(taskError));
        }
    }
    completeTaskWhenSessionEnds_ = false;

    if (taskMarkedCompleted) {
        refreshTasks();
        emit tasksChanged();
    }
    emit focusDataChanged();

    if (!completed) {
        QString status = QStringLiteral("本次%1已提前结束，实际用时 %2，已计入统计。")
                             .arg(phaseText(phase), formatSeconds(actualSeconds));
        if (taskMarkedCompleted) {
            status += QStringLiteral(" 关联任务已完成。");
        }
        statusLabel_->setText(status);
        updateIdleDuration();
        return;
    }

    playCompletionSound(phase);
    FocusTimer::Phase nextPhase = FocusTimer::Phase::Focus;
    QString notificationTitle;
    QString notificationMessage;
    bool autoStart = false;

    if (phase == FocusTimer::Phase::Focus) {
        ++completedFocusCycles_;
        const bool longBreak = completedFocusCycles_ % settings_.cyclesBeforeLongBreak == 0;
        nextPhase = longBreak ? FocusTimer::Phase::LongBreak
                              : FocusTimer::Phase::ShortBreak;
        notificationTitle = QStringLiteral("专注完成");
        notificationMessage = QStringLiteral("本次专注 %1，现在可以休息了。")
                                  .arg(formatSeconds(actualSeconds));
        autoStart = settings_.autoStartBreak;
    } else {
        nextPhase = FocusTimer::Phase::Focus;
        notificationTitle = QStringLiteral("休息结束");
        notificationMessage = QStringLiteral("精力已恢复，可以开始下一轮专注。");
        autoStart = settings_.autoStartFocus;
    }

    selectPhase(nextPhase);
    cycleLabel_->setText(QStringLiteral("当前周期：%1 / %2")
                             .arg(completedFocusCycles_ % settings_.cyclesBeforeLongBreak)
                             .arg(settings_.cyclesBeforeLongBreak));
    statusLabel_->setText(notificationMessage);
    emit notificationRequested(notificationTitle, notificationMessage);

    if (autoStart) {
        QTimer::singleShot(800, this, &FocusPage::startCurrentPhase);
    } else {
        updateIdleDuration();
    }
}

void FocusPage::updateIdleDuration()
{
    if (timer_.state() != FocusTimer::State::Idle) {
        return;
    }
    const auto phase = selectedPhase();
    phaseLabel_->setText(phaseText(phase));
    const int seconds = durationSeconds(phase);
    timerLabel_->setText(formatSeconds(seconds));
    progress_->setRange(0, qMax(1, seconds));
    progress_->setValue(0);
}

FocusTimer::Phase FocusPage::selectedPhase() const
{
    return static_cast<FocusTimer::Phase>(phaseCombo_->currentData().toInt());
}

int FocusPage::durationSeconds(FocusTimer::Phase phase) const
{
    switch (phase) {
    case FocusTimer::Phase::ShortBreak:
        return settings_.shortBreakMinutes * 60;
    case FocusTimer::Phase::LongBreak:
        return settings_.longBreakMinutes * 60;
    case FocusTimer::Phase::Focus:
    default:
        return settings_.focusMinutes * 60;
    }
}

void FocusPage::selectPhase(FocusTimer::Phase phase)
{
    const int index = phaseCombo_->findData(static_cast<int>(phase));
    if (index >= 0) {
        phaseCombo_->setCurrentIndex(index);
    }
}

void FocusPage::playCompletionSound(FocusTimer::Phase phase)
{
    if (!settings_.soundEnabled) {
        return;
    }
    const QString path = phase == FocusTimer::Phase::Focus
        ? settings_.focusSoundPath
        : settings_.breakSoundPath;
    soundPlayer_->play(path,
                       settings_.volumePercent,
                       settings_.maxSoundSeconds,
                       settings_.soundRepeatCount);
}

QString FocusPage::formatSeconds(int totalSeconds)
{
    const int safeSeconds = qMax(0, totalSeconds);
    return QStringLiteral("%1:%2")
        .arg(safeSeconds / 60, 2, 10, QLatin1Char('0'))
        .arg(safeSeconds % 60, 2, 10, QLatin1Char('0'));
}

QString FocusPage::phaseText(FocusTimer::Phase phase)
{
    switch (phase) {
    case FocusTimer::Phase::ShortBreak:
        return QStringLiteral("短休息");
    case FocusTimer::Phase::LongBreak:
        return QStringLiteral("长休息");
    case FocusTimer::Phase::Focus:
    default:
        return QStringLiteral("专注");
    }
}
