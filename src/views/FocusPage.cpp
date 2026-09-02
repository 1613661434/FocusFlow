#include "views/FocusPage.h"

#include "repositories/FocusRepository.h"
#include "repositories/SettingsRepository.h"
#include "repositories/TaskRepository.h"
#include "repositories/TimerPresetRepository.h"
#include "services/NotificationSoundPlayer.h"
#include "services/PriorityService.h"
#include "widgets/PriorityColors.h"
#include "widgets/ColoredComboBox.h"
#include "widgets/FocusAwareSpinBox.h"

#include <QComboBox>
#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QFrame>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kAllLookups = -2;
constexpr int kCustomPreset = -2;
}

FocusPage::FocusPage(QWidget *parent)
    : QWidget(parent),
      timer_(this),
      statusResetTimer_(this),
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
    statusResetTimer_.setSingleShot(true);
    statusResetTimer_.setInterval(2500);
    connect(&statusResetTimer_, &QTimer::timeout,
            this, &FocusPage::restoreIdleStatus);
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
    statusLabel_->setObjectName(QStringLiteral("focusStatusLabel"));
    statusLabel_->setProperty("muted", true);
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
    optionsCard->setFixedWidth(390);
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
    ColoredComboBox::enableCurrentItemColor(projectFilter_);
    ColoredComboBox::enableCurrentItemColor(categoryFilter_);
    ColoredComboBox::enableCurrentItemColor(taskCombo_);
    auto *presetLabel = new QLabel(QStringLiteral("专注方案"), optionsCard);
    presetCombo_ = new QComboBox(optionsCard);
    presetCombo_->setObjectName(QStringLiteral("focusPresetCombo"));
    presetCombo_->setAccessibleName(QStringLiteral("选择本次计时使用的专注方案"));
    customMinutesRow_ = new QWidget(optionsCard);
    customMinutesRow_->setObjectName(QStringLiteral("focusCustomPresetEditor"));
    customMinutesRow_->setSizePolicy(QSizePolicy::Preferred,
                                     QSizePolicy::Minimum);
    auto *customForm = new QFormLayout(customMinutesRow_);
    customForm->setContentsMargins(0, 0, 0, 4);
    customForm->setHorizontalSpacing(10);
    customForm->setVerticalSpacing(8);
    customForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    customMinutes_ = new FocusAwareSpinBox(customMinutesRow_);
    customMinutes_->setObjectName(QStringLiteral("focusCustomMinutes"));
    customMinutes_->setRange(1, 180);
    customMinutes_->setSuffix(QStringLiteral(" 分钟"));
    customMinutes_->setAccessibleName(QStringLiteral("自定义本次专注时长"));
    customShortBreakMinutes_ = new FocusAwareSpinBox(customMinutesRow_);
    customShortBreakMinutes_->setObjectName(
        QStringLiteral("focusCustomShortBreakMinutes"));
    customShortBreakMinutes_->setRange(1, 60);
    customShortBreakMinutes_->setSuffix(QStringLiteral(" 分钟"));
    customShortBreakMinutes_->setAccessibleName(QStringLiteral("自定义短休息时长"));
    customLongBreakMinutes_ = new FocusAwareSpinBox(customMinutesRow_);
    customLongBreakMinutes_->setObjectName(
        QStringLiteral("focusCustomLongBreakMinutes"));
    customLongBreakMinutes_->setRange(1, 120);
    customLongBreakMinutes_->setSuffix(QStringLiteral(" 分钟"));
    customLongBreakMinutes_->setAccessibleName(QStringLiteral("自定义长休息时长"));
    customCycles_ = new FocusAwareSpinBox(customMinutesRow_);
    customCycles_->setObjectName(QStringLiteral("focusCustomCycles"));
    customCycles_->setRange(2, 8);
    customCycles_->setSuffix(QStringLiteral(" 次专注"));
    customCycles_->setAccessibleName(QStringLiteral("自定义长休息间隔"));
    customAutoStartBreak_ = new QCheckBox(
        QStringLiteral("专注后自动开始休息"), customMinutesRow_);
    customAutoStartBreak_->setObjectName(
        QStringLiteral("focusCustomAutoStartBreak"));
    customAutoStartFocus_ = new QCheckBox(
        QStringLiteral("休息后自动开始专注"), customMinutesRow_);
    customAutoStartFocus_->setObjectName(
        QStringLiteral("focusCustomAutoStartFocus"));

    customForm->addRow(QStringLiteral("专注："), customMinutes_);
    customForm->addRow(QStringLiteral("短休息："), customShortBreakMinutes_);
    customForm->addRow(QStringLiteral("长休息："), customLongBreakMinutes_);
    customForm->addRow(QStringLiteral("长休息间隔："), customCycles_);
    customForm->addRow(customAutoStartBreak_);
    customForm->addRow(customAutoStartFocus_);

    auto *customButtons = new QWidget(customMinutesRow_);
    customButtons->setMinimumHeight(44);
    auto *customButtonsLayout = new QHBoxLayout(customButtons);
    customButtonsLayout->setContentsMargins(0, 0, 0, 3);
    customButtonsLayout->setSpacing(8);
    confirmCustomMinutesButton_ = new QPushButton(
        QStringLiteral("确定"), customButtons);
    confirmCustomMinutesButton_->setObjectName(
        QStringLiteral("confirmCustomMinutesButton"));
    confirmCustomMinutesButton_->setAccessibleName(
        QStringLiteral("确认并应用自定义本次方案"));
    confirmCustomMinutesButton_->setMinimumSize(96, 40);
    saveCustomPresetButton_ = new QPushButton(
        QStringLiteral("保存为方案"), customButtons);
    saveCustomPresetButton_->setObjectName(
        QStringLiteral("saveCustomPresetButton"));
    saveCustomPresetButton_->setAccessibleName(
        QStringLiteral("为当前自定义方案设置名称并保存"));
    saveCustomPresetButton_->setMinimumSize(138, 40);
    customButtonsLayout->addWidget(confirmCustomMinutesButton_, 2);
    customButtonsLayout->addWidget(saveCustomPresetButton_, 3);
    customForm->addRow(customButtons);
    customMinutesRow_->setVisible(false);
    setTaskPresetButton_ = new QPushButton(
        QStringLiteral("设为该任务默认方案"), optionsCard);
    setTaskPresetButton_->setObjectName(QStringLiteral("taskPresetButton"));
    setTaskPresetButton_->setAccessibleName(
        QStringLiteral("把当前方案设为关联任务的默认方案"));
    setTaskPresetButton_->setEnabled(false);
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
        QStringLiteral("方案切换只影响本次计时；需要长期使用时，可设为关联任务的"
                       "默认方案。计时使用系统时间校正。"),
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
    optionsLayout->addWidget(presetLabel);
    optionsLayout->addWidget(presetCombo_);
    optionsLayout->addWidget(customMinutesRow_);
    optionsLayout->addWidget(setTaskPresetButton_);
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
    connect(taskCombo_, &QComboBox::currentIndexChanged,
            this, &FocusPage::applyTaskPreset);
    connect(presetCombo_, &QComboBox::currentIndexChanged,
            this, &FocusPage::handlePresetChanged);
    connect(customMinutes_, &QSpinBox::valueChanged,
            this, [this] { updatePresetControls(); });
    connect(customShortBreakMinutes_, &QSpinBox::valueChanged,
            this, [this] { updatePresetControls(); });
    connect(customLongBreakMinutes_, &QSpinBox::valueChanged,
            this, [this] { updatePresetControls(); });
    connect(customCycles_, &QSpinBox::valueChanged,
            this, [this] { updatePresetControls(); });
    connect(customAutoStartBreak_, &QCheckBox::toggled,
            this, [this] { updatePresetControls(); });
    connect(customAutoStartFocus_, &QCheckBox::toggled,
            this, [this] { updatePresetControls(); });
    connect(confirmCustomMinutesButton_, &QPushButton::clicked,
            this, &FocusPage::confirmCustomMinutes);
    connect(saveCustomPresetButton_, &QPushButton::clicked,
            this, &FocusPage::saveCustomPreset);
    connect(setTaskPresetButton_, &QPushButton::clicked,
            this, &FocusPage::setSelectedPresetAsTaskDefault);

    const QVector<QWidget *> blankClickSurfaces{
        this, timerCard, phaseLabel_, timerLabel_, progress_, cycleLabel_,
        statusLabel_, optionsCard, optionsTitle, projectFilterLabel,
        categoryFilterLabel, taskLabel, presetLabel, phaseSelectLabel, tip};
    for (QWidget *surface : blankClickSurfaces) {
        surface->installEventFilter(this);
    }
    for (QLabel *label : customMinutesRow_->findChildren<QLabel *>()) {
        label->installEventFilter(this);
    }
}

void FocusPage::reloadSettings()
{
    settings_ = SettingsRepository().loadTimerSettings();
    reloadPresets();
    const TimerPreset preset = selectedPreset();
    cycleLabel_->setText(QStringLiteral("当前周期：%1 / %2")
                             .arg(completedFocusCycles_ % preset.cyclesBeforeLongBreak)
                             .arg(preset.cyclesBeforeLongBreak));
    applyTaskPreset();
    updateIdleDuration();
}

void FocusPage::reloadPresets()
{
    const int previousId = presetCombo_->currentData().isValid()
                               ? presetCombo_->currentData().toInt()
                               : -1;
    TimerPresetRepository repository;
    presets_ = repository.findAll();
    defaultPreset_ = repository.defaultPreset();
    if (defaultPreset_.id <= 0 && !presets_.isEmpty()) {
        defaultPreset_ = presets_.first();
    }

    const QSignalBlocker blocker(presetCombo_);
    presetCombo_->clear();
    for (const TimerPreset &preset : presets_) {
        QString text = QStringLiteral("%1（%2 / %3 分钟）")
                           .arg(preset.name)
                           .arg(preset.focusMinutes)
                           .arg(preset.shortBreakMinutes);
        if (preset.isDefault) {
            text += QStringLiteral(" · 默认");
        }
        presetCombo_->addItem(text, preset.id);
    }
    presetCombo_->addItem(QStringLiteral("自定义本次方案"), kCustomPreset);

    int index = presetCombo_->findData(previousId);
    if (index < 0) {
        index = presetCombo_->findData(defaultPreset_.id);
    }
    presetCombo_->setCurrentIndex(index >= 0 ? index : 0);
    const TimerPreset preset = selectedPreset();
    if (preset.id > 0) {
        loadCustomEditor(preset);
    }
    updatePresetControls();
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
        ColoredComboBox::addColoredItem(
            projectFilter_, project.name, project.id, QColor(project.color));
    }

    categoryFilter_->clear();
    categoryFilter_->addItem(QStringLiteral("全部分类"), kAllLookups);
    categoryFilter_->addItem(QStringLiteral("未分类"), -1);
    for (const LookupItem &category : repository.categories()) {
        ColoredComboBox::addColoredItem(
            categoryFilter_, category.name, category.id, QColor(category.color));
    }

    const int projectIndex = projectFilter_->findData(selectedProject);
    const int categoryIndex = categoryFilter_->findData(selectedCategory);
    projectFilter_->setCurrentIndex(projectIndex >= 0 ? projectIndex : 0);
    categoryFilter_->setCurrentIndex(categoryIndex >= 0 ? categoryIndex : 0);
    ColoredComboBox::applyCurrentItemColor(projectFilter_);
    ColoredComboBox::applyCurrentItemColor(categoryFilter_);
}

void FocusPage::refreshFilteredTasks()
{
    QSignalBlocker taskBlocker(taskCombo_);
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
    ColoredComboBox::applyCurrentItemColor(taskCombo_);
    taskBlocker.unblock();
    applyTaskPreset();
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
        ColoredComboBox::applyCurrentItemColor(projectFilter_);
        ColoredComboBox::applyCurrentItemColor(categoryFilter_);
    }
    refreshFilteredTasks();
    const int taskIndex = taskCombo_->findData(taskId);
    if (taskIndex >= 0) {
        taskCombo_->setCurrentIndex(taskIndex);
        selectPhase(FocusTimer::Phase::Focus);
        applyTaskPreset();
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
    const Task task = TaskRepository().findById(currentTaskId_);
    currentTaskTitle_ = task.id > 0 ? task.title : QStringLiteral("无关联任务");
    currentRemainingSeconds_ = durationSeconds(phase);
    timer_.start(phase, currentRemainingSeconds_, currentTaskId_);
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
    currentRemainingSeconds_ = remainingSeconds;
    updateTrayStatus();
}

void FocusPage::updateState(FocusTimer::State state)
{
    const bool idle = state == FocusTimer::State::Idle;
    stopButton_->setEnabled(!idle);
    phaseCombo_->setEnabled(idle);
    projectFilter_->setEnabled(idle);
    categoryFilter_->setEnabled(idle);
    taskCombo_->setEnabled(idle);
    presetCombo_->setEnabled(idle);
    customMinutes_->setEnabled(idle);
    customShortBreakMinutes_->setEnabled(idle);
    customLongBreakMinutes_->setEnabled(idle);
    customCycles_->setEnabled(idle);
    customAutoStartBreak_->setEnabled(idle);
    customAutoStartFocus_->setEnabled(idle);
    if (!idle) {
        statusResetTimer_.stop();
    }

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
    updatePresetControls();
    updateTrayStatus();
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
        updateIdleDuration();
        QTimer::singleShot(0, this, [this, status] {
            if (timer_.state() == FocusTimer::State::Idle) {
                showTemporaryStatus(status, 4000);
            }
        });
        return;
    }

    playCompletionSound(phase);
    FocusTimer::Phase nextPhase = FocusTimer::Phase::Focus;
    QString notificationTitle;
    QString notificationMessage;
    bool autoStart = false;

    if (phase == FocusTimer::Phase::Focus) {
        ++completedFocusCycles_;
        const TimerPreset preset = selectedPreset();
        const bool longBreak = completedFocusCycles_ % preset.cyclesBeforeLongBreak == 0;
        nextPhase = longBreak ? FocusTimer::Phase::LongBreak
                              : FocusTimer::Phase::ShortBreak;
        notificationTitle = QStringLiteral("专注完成");
        notificationMessage = QStringLiteral("本次专注 %1，现在可以休息了。")
                                  .arg(formatSeconds(actualSeconds));
        autoStart = preset.autoStartBreak;
    } else {
        nextPhase = FocusTimer::Phase::Focus;
        notificationTitle = QStringLiteral("休息结束");
        notificationMessage = QStringLiteral("精力已恢复，可以开始下一轮专注。");
        autoStart = selectedPreset().autoStartFocus;
    }

    selectPhase(nextPhase);
    cycleLabel_->setText(QStringLiteral("当前周期：%1 / %2")
                             .arg(completedFocusCycles_ % selectedPreset().cyclesBeforeLongBreak)
                             .arg(selectedPreset().cyclesBeforeLongBreak));
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
    currentRemainingSeconds_ = seconds;
    updateTrayStatus();
}

FocusTimer::Phase FocusPage::selectedPhase() const
{
    return static_cast<FocusTimer::Phase>(phaseCombo_->currentData().toInt());
}

int FocusPage::durationSeconds(FocusTimer::Phase phase) const
{
    const TimerPreset preset = selectedPreset();
    switch (phase) {
    case FocusTimer::Phase::ShortBreak:
        return preset.shortBreakMinutes * 60;
    case FocusTimer::Phase::LongBreak:
        return preset.longBreakMinutes * 60;
    case FocusTimer::Phase::Focus:
    default:
        return preset.focusMinutes * 60;
    }
}

TimerPreset FocusPage::selectedPreset() const
{
    const int id = presetCombo_ != nullptr
                       ? presetCombo_->currentData().toInt() : -1;
    if (id == kCustomPreset) {
        TimerPreset custom = confirmedCustomPreset_;
        custom.id = kCustomPreset;
        custom.name = QStringLiteral("自定义本次方案");
        custom.isDefault = false;
        custom.isBuiltIn = false;
        return custom;
    }
    for (const TimerPreset &preset : presets_) {
        if (preset.id == id) {
            return preset;
        }
    }
    return defaultPreset_;
}

void FocusPage::handlePresetChanged()
{
    const TimerPreset preset = selectedPreset();
    if (preset.id > 0) {
        loadCustomEditor(preset);
    }
    updatePresetControls();
    cycleLabel_->setText(QStringLiteral("当前周期：%1 / %2")
                             .arg(completedFocusCycles_ % preset.cyclesBeforeLongBreak)
                             .arg(preset.cyclesBeforeLongBreak));
    updateIdleDuration();
}

void FocusPage::applyTaskPreset()
{
    if (timer_.state() != FocusTimer::State::Idle || presets_.isEmpty()) {
        return;
    }
    statusResetTimer_.stop();
    restoreIdleStatus();
    const int taskId = taskCombo_->currentData().toInt();
    const Task task = TaskRepository().findById(taskId);
    const int presetId = task.id > 0 && task.timerPresetId > 0
                             ? task.timerPresetId : defaultPreset_.id;
    const int index = presetCombo_->findData(presetId);
    if (index >= 0) {
        presetCombo_->setCurrentIndex(index);
    }
    currentTaskId_ = task.id > 0 ? task.id : -1;
    currentTaskTitle_ = task.id > 0 ? task.title : QStringLiteral("无关联任务");
    updatePresetControls();
}

void FocusPage::setSelectedPresetAsTaskDefault()
{
    const int taskId = taskCombo_->currentData().toInt();
    const int presetId = presetCombo_->currentData().toInt();
    if (taskId <= 0 || presetId <= 0) {
        return;
    }
    QString error;
    if (!TaskRepository().setTimerPreset(taskId, presetId, &error)) {
        QMessageBox::warning(this, QStringLiteral("设置失败"), error);
        return;
    }
    showTemporaryStatus(
        QStringLiteral("已将“%1”设为该任务的默认专注方案。")
            .arg(selectedPreset().name));
    emit tasksChanged();
}

void FocusPage::updatePresetControls()
{
    const bool custom = presetCombo_->currentData().toInt() == kCustomPreset;
    const bool idle = timer_.state() == FocusTimer::State::Idle;
    const bool hasTask = taskCombo_->currentData().toInt() > 0;
    const bool customDirty = custom && customEditorDirty();
    customMinutesRow_->setVisible(custom);
    confirmCustomMinutesButton_->setEnabled(idle && customDirty);
    confirmCustomMinutesButton_->setToolTip(
        customDirty ? QStringLiteral("应用当前输入的完整计时方案")
                    : QStringLiteral("当前自定义方案已经确认"));
    saveCustomPresetButton_->setEnabled(idle && !customDirty);
    saveCustomPresetButton_->setToolTip(
        customDirty ? QStringLiteral("请先点击“确定”应用当前修改")
                    : QStringLiteral("输入名称，把当前配置保存为可复用方案"));
    setTaskPresetButton_->setEnabled(
        idle && hasTask && !custom);
    QString taskPresetHint;
    if (!idle) {
        taskPresetHint = QStringLiteral("计时进行中，不能修改任务默认方案");
    } else if (!hasTask) {
        taskPresetHint = QStringLiteral("请先选择一项关联任务");
    } else if (custom) {
        taskPresetHint = QStringLiteral(
            "请先将自定义方案保存为命名方案，再设为任务默认方案");
    } else {
        taskPresetHint = QStringLiteral("今后选择该任务时自动使用当前方案");
    }
    setTaskPresetButton_->setToolTip(taskPresetHint);
    setTaskPresetButton_->setAccessibleDescription(taskPresetHint);

    if (idle) {
        primaryActionButton_->setEnabled(!customDirty);
        primaryActionButton_->setToolTip(
            customDirty
                ? QStringLiteral("请先点击“确定”应用自定义方案")
                : QStringLiteral("开始当前计时"));
    }
}

void FocusPage::confirmCustomMinutes()
{
    if (timer_.state() != FocusTimer::State::Idle
        || presetCombo_->currentData().toInt() != kCustomPreset) {
        return;
    }
    confirmedCustomPreset_ = customEditorPreset();
    clearCustomEditorFocus();
    cycleLabel_->setText(QStringLiteral("当前周期：%1 / %2")
                             .arg(completedFocusCycles_
                                  % confirmedCustomPreset_.cyclesBeforeLongBreak)
                             .arg(confirmedCustomPreset_.cyclesBeforeLongBreak));
    updateIdleDuration();
    updatePresetControls();
    showTemporaryStatus(
        QStringLiteral("本次自定义方案已应用。"));
}

void FocusPage::saveCustomPreset()
{
    if (timer_.state() != FocusTimer::State::Idle
        || presetCombo_->currentData().toInt() != kCustomPreset
        || customEditorDirty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
                             this,
                             QStringLiteral("保存为专注方案"),
                             QStringLiteral("方案名称："),
                             QLineEdit::Normal,
                             QStringLiteral("我的专注方案"),
                             &accepted)
                             .trimmed();
    if (!accepted) {
        return;
    }
    if (name.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("无法保存"),
                                 QStringLiteral("请输入方案名称。"));
        return;
    }

    TimerPreset preset = confirmedCustomPreset_;
    preset.id = -1;
    preset.name = name;
    preset.isDefault = false;
    preset.isBuiltIn = false;
    QString error;
    if (!TimerPresetRepository().save(preset, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return;
    }

    reloadPresets();
    const int index = presetCombo_->findData(preset.id);
    if (index >= 0) {
        presetCombo_->setCurrentIndex(index);
    }
    emit presetsChanged();
    showTemporaryStatus(
        QStringLiteral("方案“%1”已保存。").arg(name));
}

TimerPreset FocusPage::customEditorPreset() const
{
    TimerPreset preset;
    preset.id = kCustomPreset;
    preset.name = QStringLiteral("自定义本次方案");
    preset.focusMinutes = customMinutes_->value();
    preset.shortBreakMinutes = customShortBreakMinutes_->value();
    preset.longBreakMinutes = customLongBreakMinutes_->value();
    preset.cyclesBeforeLongBreak = customCycles_->value();
    preset.autoStartBreak = customAutoStartBreak_->isChecked();
    preset.autoStartFocus = customAutoStartFocus_->isChecked();
    preset.isDefault = false;
    preset.isBuiltIn = false;
    return preset;
}

void FocusPage::loadCustomEditor(const TimerPreset &preset)
{
    confirmedCustomPreset_ = preset;
    confirmedCustomPreset_.id = kCustomPreset;
    confirmedCustomPreset_.name = QStringLiteral("自定义本次方案");
    confirmedCustomPreset_.isDefault = false;
    confirmedCustomPreset_.isBuiltIn = false;

    const QSignalBlocker focusBlocker(customMinutes_);
    const QSignalBlocker shortBreakBlocker(customShortBreakMinutes_);
    const QSignalBlocker longBreakBlocker(customLongBreakMinutes_);
    const QSignalBlocker cyclesBlocker(customCycles_);
    const QSignalBlocker autoBreakBlocker(customAutoStartBreak_);
    const QSignalBlocker autoFocusBlocker(customAutoStartFocus_);
    customMinutes_->setValue(preset.focusMinutes);
    customShortBreakMinutes_->setValue(preset.shortBreakMinutes);
    customLongBreakMinutes_->setValue(preset.longBreakMinutes);
    customCycles_->setValue(preset.cyclesBeforeLongBreak);
    customAutoStartBreak_->setChecked(preset.autoStartBreak);
    customAutoStartFocus_->setChecked(preset.autoStartFocus);
}

bool FocusPage::customEditorDirty() const
{
    return customMinutes_->value() != confirmedCustomPreset_.focusMinutes
           || customShortBreakMinutes_->value()
                  != confirmedCustomPreset_.shortBreakMinutes
           || customLongBreakMinutes_->value()
                  != confirmedCustomPreset_.longBreakMinutes
           || customCycles_->value()
                  != confirmedCustomPreset_.cyclesBeforeLongBreak
           || customAutoStartBreak_->isChecked()
                  != confirmedCustomPreset_.autoStartBreak
           || customAutoStartFocus_->isChecked()
                  != confirmedCustomPreset_.autoStartFocus;
}

void FocusPage::clearCustomEditorFocus()
{
    customMinutes_->clearFocus();
    customShortBreakMinutes_->clearFocus();
    customLongBreakMinutes_->clearFocus();
    customCycles_->clearFocus();
}

void FocusPage::showTemporaryStatus(const QString &message, int durationMs)
{
    statusLabel_->setText(message);
    statusResetTimer_.start(durationMs);
}

void FocusPage::restoreIdleStatus()
{
    if (timer_.state() == FocusTimer::State::Idle) {
        statusLabel_->setText(
            QStringLiteral("准备开始%1").arg(phaseText(selectedPhase())));
    }
}

bool FocusPage::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    QWidget *focused = QApplication::focusWidget();
    const QVector<QSpinBox *> editors{
        customMinutes_, customShortBreakMinutes_, customLongBreakMinutes_,
        customCycles_};
    const bool editingCustomValue = std::any_of(
        editors.cbegin(), editors.cend(), [focused](QSpinBox *editor) {
            return editor != nullptr
                   && (focused == editor
                       || (focused != nullptr
                           && editor->isAncestorOf(focused)));
        });
    if (event->type() == QEvent::MouseButtonPress && editingCustomValue) {
        clearCustomEditorFocus();
    }
    return QWidget::eventFilter(watched, event);
}

void FocusPage::updateTrayStatus()
{
    if (timer_.state() == FocusTimer::State::Idle) {
        emit trayStatusChanged(
            QStringLiteral("FocusFlow\n当前没有正在运行的计时"));
        return;
    }

    QString taskTitle = currentTaskTitle_.trimmed();
    if (taskTitle.isEmpty()) {
        taskTitle = QStringLiteral("无关联任务");
    }
    if (taskTitle.size() > 28) {
        taskTitle = taskTitle.left(27) + QChar(0x2026);
    }
    const bool paused = timer_.state() == FocusTimer::State::Paused;
    const FocusTimer::Phase phase = timer_.phase();
    QString stateText;
    const QString taskPrefix = QStringLiteral("任务");
    if (phase == FocusTimer::Phase::Focus) {
        stateText = paused ? QStringLiteral("专注已暂停") : QStringLiteral("专注中");
    } else {
        stateText = paused
            ? QStringLiteral("%1已暂停").arg(phaseText(phase))
            : QStringLiteral("%1中").arg(phaseText(phase));
    }
    emit trayStatusChanged(
        QStringLiteral("FocusFlow\n%1 · %2：%3\n剩余 %4")
            .arg(stateText, taskPrefix, taskTitle,
                 formatSeconds(currentRemainingSeconds_)));
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
