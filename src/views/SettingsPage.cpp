#include "views/SettingsPage.h"

#include "repositories/SettingsRepository.h"
#include "services/NotificationSoundPlayer.h"

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

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent),
      soundPlayer_(new NotificationSoundPlayer(this))
{
    buildInterface();
    loadSettings();
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
    auto *timerForm = new QFormLayout(timerGroup);
    timerForm->setHorizontalSpacing(20);
    timerForm->setVerticalSpacing(12);

    focusMinutes_ = new QSpinBox(timerGroup);
    focusMinutes_->setRange(1, 180);
    focusMinutes_->setSuffix(QStringLiteral(" 分钟"));
    shortBreakMinutes_ = new QSpinBox(timerGroup);
    shortBreakMinutes_->setRange(1, 60);
    shortBreakMinutes_->setSuffix(QStringLiteral(" 分钟"));
    longBreakMinutes_ = new QSpinBox(timerGroup);
    longBreakMinutes_->setRange(1, 120);
    longBreakMinutes_->setSuffix(QStringLiteral(" 分钟"));
    cyclesBeforeLongBreak_ = new QSpinBox(timerGroup);
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

    maxSoundSeconds_ = new QSpinBox(soundGroup);
    maxSoundSeconds_->setRange(1, 30);
    maxSoundSeconds_->setSuffix(QStringLiteral(" 秒"));
    soundRepeatCount_ = new QSpinBox(soundGroup);
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

    auto *saveButton = new QPushButton(QStringLiteral("保存设置"), content);
    saveButton->setObjectName(QStringLiteral("primaryButton"));
    saveButton->setMinimumWidth(130);
    connect(saveButton, &QPushButton::clicked, this, &SettingsPage::saveSettings);

    root->addWidget(timerGroup);
    root->addWidget(soundGroup);
    root->addWidget(saveButton, 0, Qt::AlignRight);
    root->addStretch();
    scrollArea->setWidget(content);
    outerLayout->addWidget(scrollArea);
}

void SettingsPage::loadSettings()
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
    return settings;
}

void SettingsPage::saveSettings()
{
    QString error;
    if (!SettingsRepository().saveTimerSettings(settingsFromForm(), &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("保存失败"),
                              QStringLiteral("无法保存设置：\n%1").arg(error));
        return;
    }
    emit settingsSaved();
    QMessageBox::information(this,
                             QStringLiteral("设置已保存"),
                             QStringLiteral("专注和声音设置已立即生效。"));
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
