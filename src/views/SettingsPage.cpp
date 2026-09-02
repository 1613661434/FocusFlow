#include "views/SettingsPage.h"

#include "views/AboutDialog.h"
#include "views/TimerPresetDialog.h"

#include "data/DatabaseManager.h"
#include "repositories/SettingsRepository.h"
#include "repositories/TimerPresetRepository.h"
#include "services/DataManagementService.h"
#include "services/NotificationSoundPlayer.h"
#include "services/SoundStorageService.h"
#include "widgets/FocusAwareSlider.h"
#include "widgets/FocusAwareSpinBox.h"
#include "widgets/ClearSelectionOnBlankClick.h"
#include "widgets/SortKeyTableWidgetItem.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHash>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
enum PresetColumn {
    PresetNameColumn = 0,
    FocusMinutesColumn,
    ShortBreakMinutesColumn,
    LongBreakMinutesColumn,
    CyclesColumn,
    AutoLinkColumn,
    PresetStatusColumn,
    PresetColumnCount
};

constexpr int kPresetDefaultRole = Qt::UserRole + 2;
constexpr int kPresetBuiltInRole = Qt::UserRole + 3;

int presetStatusPriority(const TimerPreset &preset)
{
    if (preset.isDefault) {
        return 0;
    }
    if (preset.isBuiltIn
        && preset.name == QStringLiteral("经典番茄钟")) {
        return 1;
    }
    return preset.isBuiltIn ? 2 : 3;
}

QString presetStatusText(const TimerPreset &preset)
{
    if (preset.isDefault) {
        if (preset.isBuiltIn
            && preset.name == QStringLiteral("经典番茄钟")) {
            return QStringLiteral("默认 · 系统默认");
        }
        return QStringLiteral("默认");
    }
    if (preset.isBuiltIn
        && preset.name == QStringLiteral("经典番茄钟")) {
        return QStringLiteral("系统默认");
    }
    return preset.isBuiltIn ? QStringLiteral("系统预设")
                            : QStringLiteral("用户自定义");
}

bool sameSettings(const TimerSettings &left, const TimerSettings &right)
{
    return left.focusMinutes == right.focusMinutes
           && left.shortBreakMinutes == right.shortBreakMinutes
           && left.longBreakMinutes == right.longBreakMinutes
           && left.cyclesBeforeLongBreak == right.cyclesBeforeLongBreak
           && left.autoStartBreak == right.autoStartBreak
           && left.autoStartFocus == right.autoStartFocus
           && left.soundEnabled == right.soundEnabled
           && left.focusSoundPath == right.focusSoundPath
           && left.breakSoundPath == right.breakSoundPath
           && left.volumePercent == right.volumePercent
           && left.maxSoundSeconds == right.maxSoundSeconds
           && left.soundRepeatCount == right.soundRepeatCount
           && left.suppressCloseToTrayReminder
                  == right.suppressCloseToTrayReminder;
}
}

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent),
      soundPlayer_(new NotificationSoundPlayer(this))
{
    buildInterface();
    reloadSettings();
}

void SettingsPage::buildInterface()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scrollArea);
    auto *root = new QVBoxLayout(content);
    root->setContentsMargins(0, 0, 18, 18);
    root->setSpacing(18);

    auto *timerGroup = new QGroupBox(QStringLiteral("专注方案"), content);
    timerGroup->setObjectName(QStringLiteral("settingsSection"));
    auto *timerLayout = new QVBoxLayout(timerGroup);
    timerLayout->setSpacing(10);

    auto *presetHint = new QLabel(
        QStringLiteral("为不同类型的任务保存不同节奏。点击任意列标题可排序，"
                       "默认依次显示默认、系统默认、系统预设和用户自定义。"
                       "任务可绑定默认方案，"
                       "计时开始前也可临时切换。内置预设不可修改或删除，"
                       "需要调整时可先复制；下列操作会立即保存。"),
        timerGroup);
    presetHint->setObjectName(QStringLiteral("mutedLabel"));
    presetHint->setWordWrap(true);

    presetTable_ = new QTableWidget(timerGroup);
    presetTable_->setObjectName(QStringLiteral("timerPresetTable"));
    presetTable_->setColumnCount(PresetColumnCount);
    presetTable_->setHorizontalHeaderLabels({
        QStringLiteral("方案"), QStringLiteral("专注"),
        QStringLiteral("短休息"), QStringLiteral("长休息"),
        QStringLiteral("长休息间隔"), QStringLiteral("自动衔接"),
        QStringLiteral("状态")});
    presetTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    presetTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    presetTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    presetTable_->setAlternatingRowColors(true);
    presetTable_->setShowGrid(false);
    presetTable_->verticalHeader()->setVisible(false);
    presetTable_->verticalHeader()->setDefaultSectionSize(42);
    presetTable_->horizontalHeader()->setSectionsClickable(true);
    presetTable_->horizontalHeader()->setSortIndicatorShown(true);
    presetTable_->horizontalHeader()->setSortIndicator(
        PresetStatusColumn, Qt::AscendingOrder);
    presetTable_->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    for (int column = 1; column < PresetColumnCount; ++column) {
        presetTable_->horizontalHeader()->setSectionResizeMode(
            column, QHeaderView::ResizeToContents);
    }
    presetTable_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    presetTable_->setSortingEnabled(true);
    presetTable_->setToolTip(
        QStringLiteral("点击任意列标题排序；默认按方案状态排序"));
    enableClearSelectionOnBlankClick(presetTable_);

    auto *presetButtons = new QHBoxLayout;
    auto *addPresetButton = new QPushButton(QStringLiteral("新建方案"), timerGroup);
    addPresetButton->setObjectName(QStringLiteral("primaryButton"));
    editPresetButton_ = new QPushButton(QStringLiteral("编辑"), timerGroup);
    copyPresetButton_ = new QPushButton(QStringLiteral("复制"), timerGroup);
    defaultPresetButton_ = new QPushButton(QStringLiteral("设为默认"), timerGroup);
    deletePresetButton_ = new QPushButton(QStringLiteral("删除"), timerGroup);
    deletePresetButton_->setObjectName(QStringLiteral("dangerButton"));
    presetButtons->addWidget(addPresetButton);
    presetButtons->addWidget(editPresetButton_);
    presetButtons->addWidget(copyPresetButton_);
    presetButtons->addWidget(defaultPresetButton_);
    presetButtons->addWidget(deletePresetButton_);
    presetButtons->addStretch();

    timerLayout->addWidget(presetHint);
    timerLayout->addWidget(presetTable_);
    timerLayout->addLayout(presetButtons);

    connect(addPresetButton, &QPushButton::clicked,
            this, &SettingsPage::addPreset);
    connect(editPresetButton_, &QPushButton::clicked,
            this, &SettingsPage::editPreset);
    connect(copyPresetButton_, &QPushButton::clicked,
            this, &SettingsPage::copyPreset);
    connect(defaultPresetButton_, &QPushButton::clicked,
            this, &SettingsPage::makePresetDefault);
    connect(deletePresetButton_, &QPushButton::clicked,
            this, &SettingsPage::deletePreset);
    connect(presetTable_, &QTableWidget::itemSelectionChanged,
            this, &SettingsPage::updatePresetButtons);
    connect(presetTable_, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { editPreset(); });

    auto *soundGroup = new QGroupBox(QStringLiteral("声音提醒"), content);
    soundGroup->setObjectName(QStringLiteral("settingsSection"));
    auto *soundForm = new QFormLayout(soundGroup);
    soundForm->setHorizontalSpacing(20);
    soundForm->setVerticalSpacing(12);

    soundEnabled_ = new QCheckBox(QStringLiteral("启用声音提醒"), soundGroup);
    focusSoundPath_ = new QLineEdit(soundGroup);
    focusSoundPath_->setReadOnly(true);
    focusSoundPath_->setPlaceholderText(QStringLiteral("使用系统默认提示音"));
    breakSoundPath_ = new QLineEdit(soundGroup);
    breakSoundPath_->setReadOnly(true);
    breakSoundPath_->setPlaceholderText(QStringLiteral("使用系统默认提示音"));

    auto createSoundRow = [soundGroup](QLineEdit *path,
                                       const QString &browseText,
                                       const QString &previewText,
                                       const QString &resetText,
                                       auto browseSlot,
                                       auto previewSlot,
                                       auto resetSlot,
                                       SettingsPage *page) {
        auto *widget = new QWidget(soundGroup);
        auto *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
        auto *browse = new QPushButton(browseText, widget);
        auto *preview = new QPushButton(previewText, widget);
        auto *reset = new QPushButton(resetText, widget);
        reset->setToolTip(QStringLiteral("清除自定义声音，恢复系统默认提示音"));
        layout->addWidget(path, 1);
        layout->addWidget(browse);
        layout->addWidget(preview);
        layout->addWidget(reset);
        QObject::connect(browse, &QPushButton::clicked, page, browseSlot);
        QObject::connect(preview, &QPushButton::clicked, page, previewSlot);
        QObject::connect(reset, &QPushButton::clicked, page, resetSlot);
        return widget;
    };

    volume_ = new FocusAwareSlider(Qt::Horizontal, soundGroup);
    volume_->setRange(0, 100);
    volumeLabel_ = new QLabel(soundGroup);
    auto *volumeWidget = new QWidget(soundGroup);
    auto *volumeLayout = new QHBoxLayout(volumeWidget);
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    volumeLayout->addWidget(volume_, 1);
    volumeLayout->addWidget(volumeLabel_);
    connect(volume_, &QSlider::valueChanged, this, [this](int value) {
        volumeLabel_->setText(QStringLiteral("%1%").arg(value));
    });

    maxSoundSeconds_ = new FocusAwareSpinBox(soundGroup);
    maxSoundSeconds_->setRange(1, 30);
    maxSoundSeconds_->setSuffix(QStringLiteral(" 秒"));
    soundRepeatCount_ = new FocusAwareSpinBox(soundGroup);
    soundRepeatCount_->setRange(1, 3);
    soundRepeatCount_->setSuffix(QStringLiteral(" 次"));

    soundForm->addRow(QString(), soundEnabled_);
    soundForm->addRow(
        QStringLiteral("专注结束声音："),
        createSoundRow(focusSoundPath_,
                       QStringLiteral("选择"),
                       QStringLiteral("试听"),
                       QStringLiteral("恢复默认"),
                       &SettingsPage::browseFocusSound,
                       &SettingsPage::previewFocusSound,
                       &SettingsPage::resetFocusSound,
                       this));
    soundForm->addRow(
        QStringLiteral("休息结束声音："),
        createSoundRow(breakSoundPath_,
                       QStringLiteral("选择"),
                       QStringLiteral("试听"),
                       QStringLiteral("恢复默认"),
                       &SettingsPage::browseBreakSound,
                       &SettingsPage::previewBreakSound,
                       &SettingsPage::resetBreakSound,
                       this));
    soundForm->addRow(QStringLiteral("提醒音量："), volumeWidget);
    soundForm->addRow(QStringLiteral("最长播放："), maxSoundSeconds_);
    soundForm->addRow(QStringLiteral("播放次数："), soundRepeatCount_);

    auto *hint = new QLabel(
        QStringLiteral("支持 WAV、MP3、AAC、M4A、OGG 和 FLAC；"
                       "达到最长播放时间前会平滑淡出，"
                       "不会修改原始音频文件。"),
        soundGroup);
    hint->setObjectName(QStringLiteral("mutedLabel"));
    hint->setWordWrap(true);
    soundForm->addRow(QString(), hint);

    auto *windowGroup = new QGroupBox(QStringLiteral("窗口行为"), content);
    windowGroup->setObjectName(QStringLiteral("settingsSection"));
    auto *windowLayout = new QVBoxLayout(windowGroup);
    windowLayout->setSpacing(8);
    suppressCloseToTrayReminder_ = new QCheckBox(
        QStringLiteral("关闭窗口隐藏到托盘时不显示提醒"),
        windowGroup);
    auto *windowHint = new QLabel(
        QStringLiteral("最小化仍保留在任务栏；关闭窗口不会退出程序，"
                       "需要从托盘图标右键菜单选择“退出”。"),
        windowGroup);
    windowHint->setObjectName(QStringLiteral("mutedLabel"));
    windowHint->setWordWrap(true);
    windowLayout->addWidget(suppressCloseToTrayReminder_);
    windowLayout->addWidget(windowHint);

    auto *dataGroup = new QGroupBox(QStringLiteral("数据管理"), content);
    dataGroup->setObjectName(QStringLiteral("settingsSection"));
    auto *dataLayout = new QVBoxLayout(dataGroup);
    dataLayout->setSpacing(12);

    auto *dataPathTitle = new QLabel(QStringLiteral("本地数据库位置"), dataGroup);
    auto *dataPath = new QLabel(DatabaseManager::instance().databasePath(), dataGroup);
    dataPath->setObjectName(QStringLiteral("mutedLabel"));
    dataPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dataPath->setWordWrap(true);

    auto *dataButtons = new QHBoxLayout;
    dataButtons->setSpacing(8);
    auto *backupButton = new QPushButton(QStringLiteral("备份数据库"), dataGroup);
    auto *restoreButton = new QPushButton(QStringLiteral("从备份恢复"), dataGroup);
    auto *exportTasksButton = new QPushButton(QStringLiteral("导出任务 CSV"), dataGroup);
    auto *exportFocusButton = new QPushButton(QStringLiteral("导出专注记录 CSV"), dataGroup);
    auto *clearStatisticsButton = new QPushButton(
        QStringLiteral("清空统计数据"), dataGroup);
    clearStatisticsButton->setObjectName(QStringLiteral("dangerButton"));
    dataButtons->addWidget(backupButton);
    dataButtons->addWidget(restoreButton);
    dataButtons->addWidget(exportTasksButton);
    dataButtons->addWidget(exportFocusButton);
    dataButtons->addWidget(clearStatisticsButton);
    dataButtons->addStretch();

    auto *dataHint = new QLabel(
        QStringLiteral("恢复操作会先自动备份当前数据，并在下次启动 FocusFlow 时生效。"),
        dataGroup);
    dataHint->setObjectName(QStringLiteral("mutedLabel"));
    dataHint->setWordWrap(true);

    dataLayout->addWidget(dataPathTitle);
    dataLayout->addWidget(dataPath);
    dataLayout->addLayout(dataButtons);
    dataLayout->addWidget(dataHint);

    connect(backupButton, &QPushButton::clicked,
            this, &SettingsPage::backupDatabase);
    connect(restoreButton, &QPushButton::clicked,
            this, &SettingsPage::restoreDatabase);
    connect(exportTasksButton, &QPushButton::clicked,
            this, &SettingsPage::exportTasks);
    connect(exportFocusButton, &QPushButton::clicked,
            this, &SettingsPage::exportFocusSessions);
    connect(clearStatisticsButton, &QPushButton::clicked,
            this, &SettingsPage::clearStatistics);

    auto *aboutGroup = new QGroupBox(QStringLiteral("关于与版权"), content);
    aboutGroup->setObjectName(QStringLiteral("settingsSection"));
    auto *aboutLayout = new QHBoxLayout(aboutGroup);
    aboutLayout->setSpacing(16);
    auto *aboutTextLayout = new QVBoxLayout;
    aboutTextLayout->setSpacing(5);
    auto *aboutTitle = new QLabel(
        QStringLiteral("FocusFlow v%1")
            .arg(QCoreApplication::applicationVersion()),
        aboutGroup);
    aboutTitle->setObjectName(QStringLiteral("aboutSettingsTitle"));
    auto *aboutSummary = new QLabel(
        QStringLiteral("作者：ol木子李lo（简称 OL） · 查看版权声明与作者博客"),
        aboutGroup);
    aboutSummary->setObjectName(QStringLiteral("mutedLabel"));
    aboutSummary->setWordWrap(true);
    aboutTextLayout->addWidget(aboutTitle);
    aboutTextLayout->addWidget(aboutSummary);
    auto *showAboutButton = new QPushButton(
        QStringLiteral("查看版权信息"), aboutGroup);
    showAboutButton->setObjectName(QStringLiteral("showCopyrightButton"));
    showAboutButton->setAccessibleName(QStringLiteral("打开版权与作者信息"));
    showAboutButton->setMinimumHeight(40);
    aboutLayout->addLayout(aboutTextLayout, 1);
    aboutLayout->addWidget(showAboutButton, 0, Qt::AlignVCenter);
    connect(showAboutButton, &QPushButton::clicked, this, [this] {
        AboutDialog dialog(this);
        dialog.exec();
    });

    auto *saveButton = new QPushButton(QStringLiteral("保存设置"), content);
    saveButton->setObjectName(QStringLiteral("primaryButton"));
    saveButton->setMinimumWidth(130);
    connect(saveButton, &QPushButton::clicked, this, [this] {
        saveSettings(true);
    });

    root->addWidget(timerGroup);
    root->addWidget(soundGroup);
    root->addWidget(windowGroup);
    root->addWidget(dataGroup);
    root->addWidget(aboutGroup);
    root->addWidget(saveButton, 0, Qt::AlignRight);
    root->addStretch();
    scrollArea->setWidget(content);
    outerLayout->addWidget(scrollArea);
}

void SettingsPage::reloadSettings()
{
    const TimerSettings settings = SettingsRepository().loadTimerSettings();
    reloadPresets();
    soundEnabled_->setChecked(settings.soundEnabled);
    focusSoundPath_->setText(settings.focusSoundPath);
    breakSoundPath_->setText(settings.breakSoundPath);
    volume_->setValue(settings.volumePercent);
    maxSoundSeconds_->setValue(settings.maxSoundSeconds);
    soundRepeatCount_->setValue(settings.soundRepeatCount);
    suppressCloseToTrayReminder_->setChecked(
        settings.suppressCloseToTrayReminder);
    savedSettings_ = settings;
}

void SettingsPage::reloadPresets()
{
    const int previousId = selectedPresetId();
    const int sortColumn = presetTable_->horizontalHeader()
                               ->sortIndicatorSection();
    const Qt::SortOrder sortOrder = presetTable_->horizontalHeader()
                                        ->sortIndicatorOrder();
    const QVector<TimerPreset> presets = TimerPresetRepository().findAll();
    presetTable_->setSortingEnabled(false);
    presetTable_->clearContents();
    presetTable_->setRowCount(presets.size());
    for (qsizetype row = 0; row < presets.size(); ++row) {
        const TimerPreset &preset = presets.at(row);
        const QString autoLink = preset.autoStartBreak && preset.autoStartFocus
                                     ? QStringLiteral("双向自动")
            : preset.autoStartBreak ? QStringLiteral("自动休息")
            : preset.autoStartFocus ? QStringLiteral("自动专注")
                                    : QStringLiteral("手动");
        const QString status = presetStatusText(preset);
        const QStringList texts{
            preset.name,
            QStringLiteral("%1 分钟").arg(preset.focusMinutes),
            QStringLiteral("%1 分钟").arg(preset.shortBreakMinutes),
            QStringLiteral("%1 分钟").arg(preset.longBreakMinutes),
            QStringLiteral("%1 次").arg(preset.cyclesBeforeLongBreak),
            autoLink,
            status};
        const QVariantList sortKeys{
            preset.name.toCaseFolded(),
            preset.focusMinutes,
            preset.shortBreakMinutes,
            preset.longBreakMinutes,
            preset.cyclesBeforeLongBreak,
            autoLink.toCaseFolded(),
            presetStatusPriority(preset)};
        for (int column = 0; column < PresetColumnCount; ++column) {
            auto *item = new SortKeyTableWidgetItem(
                texts.at(column), sortKeys.at(column));
            item->setTextAlignment(Qt::AlignCenter);
            if (column == PresetNameColumn) {
                item->setData(Qt::UserRole, preset.id);
                item->setData(kPresetDefaultRole, preset.isDefault);
                item->setData(kPresetBuiltInRole, preset.isBuiltIn);
            }
            if (preset.isBuiltIn) {
                item->setToolTip(QStringLiteral(
                    "内置预设保持固定；可使用“复制”创建可编辑方案。"));
            } else if (column == PresetStatusColumn) {
                item->setToolTip(QStringLiteral(
                    "用户自定义方案可以编辑、复制或删除。"));
            }
            presetTable_->setItem(row, column, item);
        }
    }
    presetTable_->setSortingEnabled(true);
    presetTable_->sortItems(
        sortColumn >= 0 ? sortColumn : PresetStatusColumn,
        sortColumn >= 0 ? sortOrder : Qt::AscendingOrder);

    const int tableHeight = presetTable_->horizontalHeader()->height()
                            + presetTable_->rowCount()
                                  * presetTable_->verticalHeader()
                                        ->defaultSectionSize()
                            + presetTable_->frameWidth() * 2 + 2;
    presetTable_->setFixedHeight(qMax(110, tableHeight));
    int selectedRow = -1;
    for (int row = 0; row < presetTable_->rowCount(); ++row) {
        const QTableWidgetItem *titleItem =
            presetTable_->item(row, PresetNameColumn);
        if (titleItem != nullptr
            && titleItem->data(Qt::UserRole).toInt() == previousId) {
            selectedRow = row;
            break;
        }
    }
    if (selectedRow >= 0) {
        presetTable_->selectRow(selectedRow);
    } else {
        presetTable_->clearSelection();
        presetTable_->setCurrentItem(nullptr);
    }
    updatePresetButtons();
}

int SettingsPage::selectedPresetId() const
{
    const int row = presetTable_ != nullptr ? presetTable_->currentRow() : -1;
    const QTableWidgetItem *item = row >= 0
                                       ? presetTable_->item(row, PresetNameColumn)
                                       : nullptr;
    return item != nullptr ? item->data(Qt::UserRole).toInt() : -1;
}

void SettingsPage::updatePresetButtons()
{
    const int row = presetTable_->currentRow();
    const QTableWidgetItem *item = row >= 0
                                       ? presetTable_->item(row, PresetNameColumn)
                                       : nullptr;
    const bool selected = item != nullptr;
    const bool isDefault = selected && item->data(kPresetDefaultRole).toBool();
    const bool isBuiltIn = selected && item->data(kPresetBuiltInRole).toBool();
    editPresetButton_->setEnabled(selected && !isBuiltIn);
    copyPresetButton_->setEnabled(selected);
    defaultPresetButton_->setEnabled(selected && !isDefault);
    deletePresetButton_->setEnabled(selected && !isDefault && !isBuiltIn);
    editPresetButton_->setToolTip(
        isBuiltIn ? QStringLiteral("内置预设不能修改，请先复制一份")
                  : QStringLiteral("编辑所选用户方案"));
    deletePresetButton_->setToolTip(
        isBuiltIn ? QStringLiteral("内置预设不能删除")
        : isDefault ? QStringLiteral("默认方案不能删除，请先把其他方案设为默认")
                    : QStringLiteral("删除方案；绑定任务将改为跟随默认方案"));
}

void SettingsPage::addPreset()
{
    TimerPreset preset;
    preset.id = -1;
    preset.name = QStringLiteral("新专注方案");
    preset.isDefault = false;
    TimerPresetDialog dialog(this);
    dialog.setPreset(preset);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    preset = dialog.preset();
    QString error;
    if (!TimerPresetRepository().save(preset, &error)) {
        QMessageBox::warning(this, QStringLiteral("新建失败"), error);
        return;
    }
    reloadPresets();
    emit settingsSaved();
}

void SettingsPage::editPreset()
{
    TimerPreset preset = TimerPresetRepository().findById(selectedPresetId());
    if (preset.id <= 0) {
        return;
    }
    if (preset.isBuiltIn) {
        QMessageBox::information(
            this, QStringLiteral("内置预设"),
            QStringLiteral("内置预设保持固定，不能直接修改。\n"
                           "请点击“复制”，再编辑生成的用户方案。"));
        return;
    }
    TimerPresetDialog dialog(this);
    dialog.setPreset(preset);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    preset = dialog.preset();
    QString error;
    if (!TimerPresetRepository().save(preset, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return;
    }
    reloadPresets();
    emit settingsSaved();
}

void SettingsPage::copyPreset()
{
    TimerPreset preset = TimerPresetRepository().findById(selectedPresetId());
    if (preset.id <= 0) {
        return;
    }
    preset.id = -1;
    preset.name += QStringLiteral(" 副本");
    preset.isDefault = false;
    preset.isBuiltIn = false;
    TimerPresetDialog dialog(this);
    dialog.setPreset(preset);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    preset = dialog.preset();
    QString error;
    if (!TimerPresetRepository().save(preset, &error)) {
        QMessageBox::warning(this, QStringLiteral("复制失败"), error);
        return;
    }
    reloadPresets();
    emit settingsSaved();
}

void SettingsPage::makePresetDefault()
{
    QString error;
    if (!TimerPresetRepository().setDefault(selectedPresetId(), &error)) {
        QMessageBox::warning(this, QStringLiteral("设置失败"), error);
        return;
    }
    reloadPresets();
    emit settingsSaved();
}

void SettingsPage::deletePreset()
{
    const int row = presetTable_->currentRow();
    const QTableWidgetItem *item = row >= 0
                                       ? presetTable_->item(row, PresetNameColumn)
                                       : nullptr;
    if (item == nullptr) {
        return;
    }
    const auto choice = QMessageBox::question(
        this, QStringLiteral("删除专注方案"),
        QStringLiteral("确定删除“%1”吗？\n绑定该方案的任务将改为跟随默认方案。")
            .arg(item->text()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        return;
    }
    QString error;
    if (!TimerPresetRepository().remove(item->data(Qt::UserRole).toInt(), &error)) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), error);
        return;
    }
    reloadPresets();
    emit settingsSaved();
}

bool SettingsPage::hasUnsavedChanges() const
{
    return !sameSettings(settingsFromForm(), savedSettings_);
}

TimerSettings SettingsPage::settingsFromForm() const
{
    TimerSettings settings = savedSettings_;
    settings.soundEnabled = soundEnabled_->isChecked();
    settings.focusSoundPath = focusSoundPath_->text();
    settings.breakSoundPath = breakSoundPath_->text();
    settings.volumePercent = volume_->value();
    settings.maxSoundSeconds = maxSoundSeconds_->value();
    settings.soundRepeatCount = soundRepeatCount_->value();
    settings.suppressCloseToTrayReminder =
        suppressCloseToTrayReminder_->isChecked();
    return settings;
}

bool SettingsPage::saveSettings(bool showConfirmation)
{
    TimerSettings settings = settingsFromForm();
    SoundStorageService storage;
    QStringList newlyInstalled;
    QHash<QString, QString> installedSources;
    QString error;

    auto prepareSound = [&](QString &path, const QString &prefix) {
        if (path.isEmpty() || storage.isManagedPath(path)) {
            return true;
        }

        const QString sourceKey = QFileInfo(path).absoluteFilePath();
        if (installedSources.contains(sourceKey)) {
            path = installedSources.value(sourceKey);
            return true;
        }

        const QString installed = storage.install(path, prefix, &error);
        if (installed.isEmpty()) {
            return false;
        }
        installedSources.insert(sourceKey, installed);
        newlyInstalled.append(installed);
        path = installed;
        return true;
    };

    if (!prepareSound(settings.focusSoundPath, QStringLiteral("focus"))
        || !prepareSound(settings.breakSoundPath, QStringLiteral("break"))) {
        for (const QString &path : newlyInstalled) {
            QFile::remove(path);
        }
        QMessageBox::critical(this,
                              QStringLiteral("保存失败"),
                              QStringLiteral("无法保存提醒声音：\n%1").arg(error));
        return false;
    }

    if (!SettingsRepository().saveTimerSettings(settings, &error)) {
        for (const QString &path : newlyInstalled) {
            QFile::remove(path);
        }
        QMessageBox::critical(this,
                              QStringLiteral("保存失败"),
                              QStringLiteral("无法保存设置：\n%1").arg(error));
        return false;
    }

    focusSoundPath_->setText(settings.focusSoundPath);
    breakSoundPath_->setText(settings.breakSoundPath);
    savedSettings_ = settings;
    soundPlayer_->stop();
    const QStringList cleanupFailures = storage.removeUnused(
        {settings.focusSoundPath, settings.breakSoundPath});
    emit settingsSaved();
    if (showConfirmation) {
        if (cleanupFailures.isEmpty()) {
            QMessageBox::information(
                this,
                QStringLiteral("设置已保存"),
                QStringLiteral("声音和窗口设置已立即生效。专注方案会在管理时立即保存。"));
        } else {
            QMessageBox::warning(
                this,
                QStringLiteral("设置已保存"),
                QStringLiteral("设置已生效，但有 %1 个旧声音文件暂时无法清理，"
                               "程序会在下次保存设置时重试。")
                    .arg(cleanupFailures.size()));
        }
    }
    return true;
}

void SettingsPage::browseFocusSound()
{
    browseSound(focusSoundPath_);
}

void SettingsPage::browseBreakSound()
{
    browseSound(breakSoundPath_);
}

void SettingsPage::resetFocusSound()
{
    focusSoundPath_->clear();
}

void SettingsPage::resetBreakSound()
{
    breakSoundPath_->clear();
}

void SettingsPage::previewFocusSound()
{
    previewSound(focusSoundPath_->text());
}

void SettingsPage::previewBreakSound()
{
    previewSound(breakSoundPath_->text());
}

void SettingsPage::backupDatabase()
{
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultPath = QDir(documents).filePath(
        QStringLiteral("FocusFlow-backup-%1.db")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    const QString destination = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("备份 FocusFlow 数据"),
        defaultPath,
        QStringLiteral("FocusFlow 数据库 (*.db)"));
    if (destination.isEmpty()) {
        return;
    }

    QString error;
    if (!DataManagementService().backupDatabase(destination, &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("备份失败"),
                              QStringLiteral("无法创建备份：\n%1").arg(error));
        return;
    }
    QMessageBox::information(this,
                             QStringLiteral("备份完成"),
                             QStringLiteral("数据已备份到：\n%1").arg(destination));
}

void SettingsPage::restoreDatabase()
{
    const QString source = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择 FocusFlow 备份"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QStringLiteral("FocusFlow 数据库 (*.db);;所有文件 (*.*)"));
    if (source.isEmpty()) {
        return;
    }

    const auto choice = QMessageBox::warning(
        this,
        QStringLiteral("确认恢复数据"),
        QStringLiteral("下次启动时，当前任务、项目、设置和专注记录将被备份文件替换。"
                       "\n\nFocusFlow 会先自动备份现有数据。是否继续？"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (choice != QMessageBox::Yes) {
        return;
    }

    QString recoveryPath;
    QString error;
    if (!DataManagementService().prepareRestore(source, &recoveryPath, &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("无法恢复"),
                              QStringLiteral("备份验证或准备失败：\n%1").arg(error));
        return;
    }
    QMessageBox::information(
        this,
        QStringLiteral("恢复已准备"),
        QStringLiteral("请退出并重新打开 FocusFlow，恢复的数据就会生效。"
                       "\n\n当前数据的自动备份：\n%1")
            .arg(recoveryPath));
}

void SettingsPage::exportTasks()
{
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultPath = QDir(documents).filePath(
        QStringLiteral("FocusFlow-tasks-%1.csv")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"))));
    const QString destination = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出任务"),
        defaultPath,
        QStringLiteral("CSV 表格 (*.csv)"));
    if (destination.isEmpty()) {
        return;
    }

    QString error;
    if (!DataManagementService().exportTasksCsv(destination, &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("导出失败"),
                              QStringLiteral("无法导出任务：\n%1").arg(error));
        return;
    }
    QMessageBox::information(this,
                             QStringLiteral("导出完成"),
                             QStringLiteral("任务已导出到：\n%1").arg(destination));
}

void SettingsPage::exportFocusSessions()
{
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultPath = QDir(documents).filePath(
        QStringLiteral("FocusFlow-focus-sessions-%1.csv")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"))));
    const QString destination = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出专注记录"),
        defaultPath,
        QStringLiteral("CSV 表格 (*.csv)"));
    if (destination.isEmpty()) {
        return;
    }

    QString error;
    if (!DataManagementService().exportFocusSessionsCsv(destination, &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("导出失败"),
                              QStringLiteral("无法导出专注记录：\n%1").arg(error));
        return;
    }
    QMessageBox::information(
        this,
        QStringLiteral("导出完成"),
        QStringLiteral("专注记录已导出到：\n%1").arg(destination));
}

void SettingsPage::clearStatistics()
{
    const auto choice = QMessageBox::warning(
        this,
        QStringLiteral("确认清空统计数据"),
        QStringLiteral("将永久删除全部专注和休息记录，今日专注、近7天专注和分类专注统计会归零。\n\n"
                       "任务、项目、分类和任务完成状态不会受到影响。此操作无法撤销，是否继续？"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (choice != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!DataManagementService().clearFocusStatistics(&error)) {
        QMessageBox::critical(this,
                              QStringLiteral("清空失败"),
                              QStringLiteral("无法清空统计数据：\n%1").arg(error));
        return;
    }
    emit statisticsCleared();
    QMessageBox::information(this,
                             QStringLiteral("统计数据已清空"),
                             QStringLiteral("全部专注和休息记录已删除，任务数据已保留。"));
}

void SettingsPage::browseSound(QLineEdit *destination)
{
    const QString sourcePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择提醒声音"),
        {},
        QStringLiteral("音频文件 (*.wav *.mp3 *.aac *.m4a *.ogg *.flac);;所有文件 (*.*)"));
    if (sourcePath.isEmpty()) {
        return;
    }
    destination->setText(sourcePath);
}

void SettingsPage::previewSound(const QString &path)
{
    if (!soundEnabled_->isChecked()) {
        QMessageBox::information(this,
                                 QStringLiteral("声音已关闭"),
                                 QStringLiteral("请先启用声音提醒。"));
        return;
    }
    soundPlayer_->play(path,
                       volume_->value(),
                       maxSoundSeconds_->value(),
                       soundRepeatCount_->value());
}
