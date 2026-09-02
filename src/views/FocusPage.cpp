#include "views/FocusPage.h"

#include "repositories/FocusRepository.h"
#include "repositories/SettingsRepository.h"
#include "repositories/TaskRepository.h"
#include "services/NotificationSoundPlayer.h"
#include "services/PriorityService.h"
#include "widgets/PriorityColors.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kAllLookups = -2;
}

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
    controls->setSpacing(12);
    primaryActionButton_ = new QPushButton(QStringLiteral("开始"), timerCard);
    primaryActionButton_->setObjectName(QStringLiteral("primaryButton"));
    primaryActionButton_->setProperty("timerControl", QStringLiteral("primary"));
    primaryActionButton_->setAccessibleName(QStringLiteral("计时主操作"));
    primaryActionButton_->setToolTip(QStringLiteral("开始当前计时"));
    primaryActionButton_->setMinimumSize(104, 44);
    stopButton_ = new QPushButton(QStringLiteral("终止"), timerCard);
    stopButton_->setObjectName(QStringLiteral("dangerButton"));
    stopButton_->setProperty("timerControl", QStringLiteral("stop"));
    stopButton_->setAccessibleName(QStringLiteral("终止当前计时"));
    stopButton_->setToolTip(QStringLiteral("终止当前计时并处理本次记录"));
    stopButton_->setMinimumSize(104, 44);
    stopButton_->setEnabled(false);
    controls->addStretch();
    controls->addWidget(primaryActionButton_);
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
    auto *projectFilterLabel = new QLabel(QStringLiteral("项目筛选"), optionsCard);
    projectFilter_ = new QComboBox(optionsCard);
    projectFilter_->setObjectName(QStringLiteral("focusProjectFilter"));
    projectFilter_->setAccessibleName(QStringLiteral("按项目筛选关联任务"));
    auto *categoryFilterLabel = new QLabel(QStringLiteral("分类筛选"), optionsCard);
    categoryFilter_ = new QComboBox(optionsCard);
    categoryFilter_->setObjectName(QStringLiteral("focusCategoryFilter"));
    categoryFilter_->setAccessibleName(QStringLiteral("按分类筛选关联任务"));
    auto *taskLabel = new QLabel(QStringLiteral("关联任务"), optionsCard);
    taskCombo_ = new QComboBox(optionsCard);
    taskCombo_->setObjectName(QStringLiteral("focusTaskCombo"));
    taskCombo_->setAccessibleName(QStringLiteral("关联任务，按推荐分从高到低排列"));
    auto *phaseSelectLabel = new QLabel(QStringLiteral("计时类型"), optionsCard);
    phaseCombo_ = new QComboBox(optionsCard);
    phaseCombo_->setObjectName(QStringLiteral("focusPhaseCombo"));
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
    optionsLayout->addWidget(projectFilterLabel);
    optionsLayout->addWidget(projectFilter_);
    optionsLayout->addWidget(categoryFilterLabel);
    optionsLayout->addWidget(categoryFilter_);
    optionsLayout->addWidget(taskLabel);
    optionsLayout->addWidget(taskCombo_);
    optionsLayout->addWidget(phaseSelectLabel);
    optionsLayout->addWidget(phaseCombo_);
    optionsLayout->addSpacing(10);
    optionsLayout->addWidget(tip);
    optionsLayout->addStretch();

    root->addWidget(timerCard, 1);
    root->addWidget(optionsCard);

    connect(primaryActionButton_, &QPushButton::clicked,
            this, &FocusPage::handlePrimaryAction);
    connect(stopButton_, &QPushButton::clicked,
            this, &FocusPage::stopEarly);
    connect(phaseCombo_, &QComboBox::currentIndexChanged,
            this, &FocusPage::updateIdleDuration);
    connect(projectFilter_, &QComboBox::currentIndexChanged,
            this, &FocusPage::refreshFilteredTasks);
    connect(categoryFilter_, &QComboBox::currentIndexChanged,
            this, &FocusPage::refreshFilteredTasks);
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
    reloadTaskFilters();
    refreshFilteredTasks();
}

void FocusPage::reloadTaskFilters()
{
    const int selectedProject = projectFilter_->currentData().isValid()
                                    ? projectFilter_->currentData().toInt()
                                    : kAllLookups;
    const int selectedCategory = categoryFilter_->currentData().isValid()
                                     ? categoryFilter_->currentData().toInt()
                                     : kAllLookups;
    const QSignalBlocker projectBlocker(projectFilter_);
    const QSignalBlocker categoryBlocker(categoryFilter_);

    TaskRepository repository;
    projectFilter_->clear();
    projectFilter_->addItem(QStringLiteral("全部项目"), kAllLookups);
    projectFilter_->addItem(QStringLiteral("无项目"), -1);
    for (const LookupItem &project : repository.projects()) {
        projectFilter_->addItem(project.name, project.id);
    }

    categoryFilter_->clear();
    categoryFilter_->addItem(QStringLiteral("全部分类"), kAllLookups);
    categoryFilter_->addItem(QStringLiteral("未分类"), -1);
    for (const LookupItem &category : repository.categories()) {
        categoryFilter_->addItem(category.name, category.id);
    }

    const int projectIndex = projectFilter_->findData(selectedProject);
    const int categoryIndex = categoryFilter_->findData(selectedCategory);
    projectFilter_->setCurrentIndex(projectIndex >= 0 ? projectIndex : 0);
    categoryFilter_->setCurrentIndex(categoryIndex >= 0 ? categoryIndex : 0);
}

void FocusPage::refreshFilteredTasks()
{
    const int previousId = taskCombo_->currentData().isValid()
                               ? taskCombo_->currentData().toInt()
                               : -1;
    const int selectedProject = projectFilter_->currentData().toInt();
    const int selectedCategory = categoryFilter_->currentData().toInt();

    auto tasks = TaskRepository().findAll(TaskRepository::Filter::All);
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [&](const Task &task) {
        const bool unavailable = task.status == QStringLiteral("completed")
                                 || task.status == QStringLiteral("cancelled");
        const bool projectMismatch = selectedProject != kAllLookups
                                     && task.projectId != selectedProject;
        const bool categoryMismatch = selectedCategory != kAllLookups
                                      && task.categoryId != selectedCategory;
        return unavailable || projectMismatch || categoryMismatch;
    }), tasks.end());
    std::stable_sort(tasks.begin(), tasks.end(), [](const Task &left,
                                                    const Task &right) {
        return PriorityService::score(left) > PriorityService::score(right);
    });

    taskCombo_->clear();
    taskCombo_->addItem(QStringLiteral("无关联任务"), -1);
    for (const auto &task : tasks) {
        const int score = PriorityService::score(task);
        taskCombo_->addItem(
            QStringLiteral("%1（推荐分 %2）").arg(task.title).arg(score),
            task.id);
        const int itemIndex = taskCombo_->count() - 1;
        taskCombo_->setItemData(itemIndex,
                                PriorityColors::recommendation(score),
                                Qt::ForegroundRole);
        const QString projectName = task.projectName.isEmpty()
                                        ? QStringLiteral("无项目")
                                        : task.projectName;
        const QString categoryName = task.categoryName.isEmpty()
                                         ? QStringLiteral("未分类")
                                         : task.categoryName;
        taskCombo_->setItemData(
            itemIndex,
            QStringLiteral("项目：%1\n分类：%2\n推荐分：%3")
                .arg(projectName, categoryName, QString::number(score)),
            Qt::ToolTipRole);
    }
    const int previousIndex = taskCombo_->findData(previousId);
    taskCombo_->setCurrentIndex(previousIndex >= 0 ? previousIndex : 0);
}

void FocusPage::selectTask(int taskId)
{
    if (timer_.state() != FocusTimer::State::Idle) {
        return;
    }
    const Task task = TaskRepository().findById(taskId);
    reloadTaskFilters();
    if (task.id <= 0 || task.status == QStringLiteral("completed")
        || task.status == QStringLiteral("cancelled")) {
        refreshFilteredTasks();
        return;
    }

    {
        const QSignalBlocker projectBlocker(projectFilter_);
        const QSignalBlocker categoryBlocker(categoryFilter_);
        const int projectIndex = projectFilter_->findData(task.projectId);
        const int categoryIndex = categoryFilter_->findData(task.categoryId);
        projectFilter_->setCurrentIndex(projectIndex >= 0 ? projectIndex : 0);
        categoryFilter_->setCurrentIndex(categoryIndex >= 0 ? categoryIndex : 0);
    }
    refreshFilteredTasks();
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

void FocusPage::handlePrimaryAction()
{
    if (timer_.state() == FocusTimer::State::Idle) {
        startCurrentPhase();
    } else if (timer_.state() == FocusTimer::State::Running) {
        timer_.pause();
    } else if (timer_.state() == FocusTimer::State::Paused) {
        timer_.resume();
    }
}

void FocusPage::stopEarly()
{
    completeTaskWhenSessionEnds_ = false;
    const FocusTimer::Phase phase = timer_.phase();
    const bool hasLinkedTask = phase == FocusTimer::Phase::Focus
        && currentTaskId_ > 0;

    if (hasLinkedTask) {
        QMessageBox dialog(QMessageBox::Question,
                           QStringLiteral("终止专注"),
                           QStringLiteral("实际用时会保存并计入专注统计。\n"
                                          "是否同时将关联任务标记为完成？"),
                           QMessageBox::NoButton,
                           this);
        auto *finishOnlyButton = dialog.addButton(
            QStringLiteral("仅终止计时"), QMessageBox::AcceptRole);
        auto *completeTaskButton = dialog.addButton(
            QStringLiteral("终止并完成任务"), QMessageBox::ActionRole);
        auto *cancelButton = dialog.addButton(
            QStringLiteral("取消"), QMessageBox::RejectRole);
        dialog.setDefaultButton(cancelButton);
        dialog.exec();

        if (dialog.clickedButton() == cancelButton
            || dialog.clickedButton() == nullptr) {
            return;
        }
        completeTaskWhenSessionEnds_ =
            dialog.clickedButton() == completeTaskButton;
        Q_UNUSED(finishOnlyButton);
    } else if (phase == FocusTimer::Phase::Focus) {
        const auto choice = QMessageBox::question(
            this,
            QStringLiteral("终止专注"),
            QStringLiteral("要终止当前专注并保存实际用时吗？\n"
                           "已产生的专注时间会计入统计。"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return;
        }
    } else {
        const auto choice = QMessageBox::question(
            this,
            QStringLiteral("终止休息"),
            QStringLiteral("要终止当前%1吗？\n"
                           "休息时间不会计入专注统计。")
                .arg(phaseText(phase)),
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
    stopButton_->setEnabled(!idle);
    phaseCombo_->setEnabled(idle);
    projectFilter_->setEnabled(idle);
    categoryFilter_->setEnabled(idle);
    taskCombo_->setEnabled(idle);

    if (state == FocusTimer::State::Running) {
        primaryActionButton_->setText(QStringLiteral("暂停"));
        primaryActionButton_->setToolTip(QStringLiteral("暂停当前计时"));
        phaseLabel_->setText(phaseText(timer_.phase()));
        statusLabel_->setText(QStringLiteral("正在%1……").arg(phaseText(timer_.phase())));
    } else if (state == FocusTimer::State::Paused) {
        primaryActionButton_->setText(QStringLiteral("继续"));
        primaryActionButton_->setToolTip(QStringLiteral("继续当前计时"));
        statusLabel_->setText(QStringLiteral("计时已暂停"));
    } else {
        primaryActionButton_->setText(QStringLiteral("开始"));
        primaryActionButton_->setToolTip(QStringLiteral("开始当前计时"));
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
        completed ? QString() : QStringLiteral("用户终止"),
        &error);
    if (!saved) {
        completeTaskWhenSessionEnds_ = false;
        statusLabel_->setText(QStringLiteral("计时记录保存失败：%1").arg(error));
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
        QString status;
        if (phase == FocusTimer::Phase::Focus) {
            status = QStringLiteral("本次专注已终止，实际用时 %1，"
                                    "已计入专注统计。")
                         .arg(formatSeconds(actualSeconds));
        } else {
            status = QStringLiteral("本次%1已终止，实际用时 %2；"
                                    "休息时间未计入专注统计。")
                         .arg(phaseText(phase), formatSeconds(actualSeconds));
        }
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
