#pragma once

#include "models/TimerSettings.h"

#include <QWidget>

class NotificationSoundPlayer;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSlider;
class QSpinBox;

class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    bool hasUnsavedChanges() const;
    bool saveSettings(bool showConfirmation = true);

signals:
    void settingsSaved();
    void statisticsCleared();

public slots:
    void reloadSettings();

private slots:
    void browseFocusSound();
    void browseBreakSound();
    void previewFocusSound();
    void previewBreakSound();
    void backupDatabase();
    void restoreDatabase();
    void exportTasks();
    void exportFocusSessions();
    void clearStatistics();

private:
    void buildInterface();
    TimerSettings settingsFromForm() const;
    void browseSound(QLineEdit *destination, const QString &prefix);
    QString installSound(const QString &sourcePath, const QString &prefix) const;
    void previewSound(const QString &path);

    NotificationSoundPlayer *soundPlayer_ = nullptr;
    QSpinBox *focusMinutes_ = nullptr;
    QSpinBox *shortBreakMinutes_ = nullptr;
    QSpinBox *longBreakMinutes_ = nullptr;
    QSpinBox *cyclesBeforeLongBreak_ = nullptr;
    QCheckBox *autoStartBreak_ = nullptr;
    QCheckBox *autoStartFocus_ = nullptr;
    QCheckBox *soundEnabled_ = nullptr;
    QLineEdit *focusSoundPath_ = nullptr;
    QLineEdit *breakSoundPath_ = nullptr;
    QSlider *volume_ = nullptr;
    QLabel *volumeLabel_ = nullptr;
    QSpinBox *maxSoundSeconds_ = nullptr;
    QSpinBox *soundRepeatCount_ = nullptr;
    QCheckBox *suppressCloseToTrayReminder_ = nullptr;
    TimerSettings savedSettings_;
};
