#include "views/SettingsPage.h"

#include "data/DatabaseManager.h"
#include "repositories/SettingsRepository.h"
#include "services/DataManagementService.h"
#include "services/NotificationSoundPlayer.h"
#include "widgets/FocusAwareSpinBox.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
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

    auto *timerGroup = new QGroupBox(QStringLiteral("专注与休息"), content);
    timerGroup->setObjectName(QStringLiteral("settingsSection"));
    auto *timerForm = new QFormLayout(timerGroup);
    timerForm->setHorizontalSpacing(20);
    timerForm->setVerticalSpacing(12);

    focusMinutes_ = new FocusAwareSpinBox(timerGroup);
    focusMinutes_->setRange(1, 180);
    focusMinutes_->setSuffix(QStringLiteral(" 分钟"));
    shortBreakMinutes_ = new FocusAwareSpinBox(timerGroup);
    shortBreakMinutes_->setRange(1, 60);
    shortBreakMinutes_->setSuffix(QStringLiteral(" 分钟"));
    longBreakMinutes_ = new FocusAwareSpinBox(timerGroup);
    longBreakMinutes_->setRange(1, 120);
    longBreakMinutes_->setSuffix(QStringLiteral(" 分钟"));
    cyclesBeforeLongBreak_ = new FocusAwareSpinBox(timerGroup);
    cyclesBeforeLongBreak_->setRange(2, 8);
    cyclesBeforeLongBreak_->setSuffix(QStringLiteral(" 次专注"));
    autoStartBreak_ = new QCheckBox(QStringLiteral("专注完成后自动开始休息"), timerGroup);
    autoStartFocus_ = new QCheckBox(QStringLiteral("休息完成后自动开始专注"), timerGroup);

    timerForm->addRow(QStringLiteral("专注时长："), focusMinutes_);
    timerForm->addRow(QStringLiteral("短休息："), shortBreakMinutes_);
    timerForm->addRow(QStringLiteral("长休息："), longBreakMinutes_);
    timerForm->addRow(QStringLiteral("长休息间隔："), cyclesBeforeLongBreak_);
    timerForm->addRow(QString(), autoStartBreak_);
    timerForm->addRow(QString(), autoStartFocus_);

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
                                       auto browseSlot,
                                       auto previewSlot,
                                       SettingsPage *page) {
        auto *widget = new QWidget(soundGroup);
        auto *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
        auto *browse = new QPushButton(browseText, widget);
        auto *preview = new QPushButton(previewText, widget);
        layout->addWidget(path, 1);
        layout->addWidget(browse);
        layout->addWidget(preview);
        QObject::connect(browse, &QPushButton::clicked, page, browseSlot);
        QObject::connect(preview, &QPushButton::clicked, page, previewSlot);
        return widget;
    };

    volume_ = new QSlider(Qt::Horizontal, soundGroup);
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
                       &SettingsPage::browseFocusSound,
                       &SettingsPage::previewFocusSound,
                       this));
    soundForm->addRow(
        QStringLiteral("休息结束声音："),
        createSoundRow(breakSoundPath_,
                       QStringLiteral("选择"),
                       QStringLiteral("试听"),
                       &SettingsPage::browseBreakSound,
                       &SettingsPage::previewBreakSound,
                       this));
    soundForm->addRow(QStringLiteral("提醒音量："), volumeWidget);
    soundForm->addRow(QStringLiteral("最长播放："), maxSoundSeconds_);
    soundForm->addRow(QStringLiteral("播放次数："), soundRepeatCount_);

    auto *hint = new QLabel(
        QStringLiteral("支持 WAV、MP3、AAC、M4A、OGG 和 FLAC；"
                       "超过最长播放时间的部分会在播放时被截止，"
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
    root->addWidget(saveButton, 0, Qt::AlignRight);
    root->addStretch();
    scrollArea->setWidget(content);
    outerLayout->addWidget(scrollArea);
}

void SettingsPage::reloadSettings()
{
    const TimerSettings settings = SettingsRepository().loadTimerSettings();
    focusMinutes_->setValue(settings.focusMinutes);
    shortBreakMinutes_->setValue(settings.shortBreakMinutes);
    longBreakMinutes_->setValue(settings.longBreakMinutes);
    cyclesBeforeLongBreak_->setValue(settings.cyclesBeforeLongBreak);
    autoStartBreak_->setChecked(settings.autoStartBreak);
    autoStartFocus_->setChecked(settings.autoStartFocus);
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

bool SettingsPage::hasUnsavedChanges() const
{
    return !sameSettings(settingsFromForm(), savedSettings_);
}

TimerSettings SettingsPage::settingsFromForm() const
{
    TimerSettings settings;
    settings.focusMinutes = focusMinutes_->value();
    settings.shortBreakMinutes = shortBreakMinutes_->value();
    settings.longBreakMinutes = longBreakMinutes_->value();
    settings.cyclesBeforeLongBreak = cyclesBeforeLongBreak_->value();
    settings.autoStartBreak = autoStartBreak_->isChecked();
    settings.autoStartFocus = autoStartFocus_->isChecked();
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
    const TimerSettings settings = settingsFromForm();
    QString error;
    if (!SettingsRepository().saveTimerSettings(settings, &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("保存失败"),
                              QStringLiteral("无法保存设置：\n%1").arg(error));
        return false;
    }
    savedSettings_ = settings;
    emit settingsSaved();
    if (showConfirmation) {
        QMessageBox::information(this,
                                 QStringLiteral("设置已保存"),
                                 QStringLiteral("专注、声音和窗口设置已立即生效。"));
    }
    return true;
}

void SettingsPage::browseFocusSound()
{
    browseSound(focusSoundPath_, QStringLiteral("focus"));
}

void SettingsPage::browseBreakSound()
{
    browseSound(breakSoundPath_, QStringLiteral("break"));
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

void SettingsPage::browseSound(QLineEdit *destination, const QString &prefix)
{
    const QString sourcePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择提醒声音"),
        {},
        QStringLiteral("音频文件 (*.wav *.mp3 *.aac *.m4a *.ogg *.flac);;所有文件 (*.*)"));
    if (sourcePath.isEmpty()) {
        return;
    }
    destination->setText(installSound(sourcePath, prefix));
}

QString SettingsPage::installSound(const QString &sourcePath, const QString &prefix) const
{
    const QFileInfo source(sourcePath);
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir soundDirectory(QDir(dataPath).filePath(QStringLiteral("sounds")));
    if (!soundDirectory.mkpath(QStringLiteral("."))) {
        return sourcePath;
    }

    const QString fileName = QStringLiteral("%1-%2.%3")
                                 .arg(prefix)
                                 .arg(QDateTime::currentMSecsSinceEpoch())
                                 .arg(source.suffix().toLower());
    const QString destination = soundDirectory.filePath(fileName);
    return QFile::copy(sourcePath, destination) ? destination : sourcePath;
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
