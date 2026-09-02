#pragma once

#include "models/TimerSettings.h"

#include <QWidget>

class NotificationSoundPlayer;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSlider;
class QSpinBox;
class QTableWidget;
class QPushButton;

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
    void reloadPresets();

private slots:
    void browseFocusSound();
    void browseBreakSound();
    void resetFocusSound();
    void resetBreakSound();
    void previewFocusSound();
    void previewBreakSound();
    void backupDatabase();
    void restoreDatabase();
    void exportTasks();
    void exportFocusSessions();
    void clearStatistics();
    void addPreset();
    void editPreset();
    void copyPreset();
    void makePresetDefault();
    void deletePreset();

private:
    void buildInterface();
    TimerSettings settingsFromForm() const;
    void browseSound(QLineEdit *destination);
    void previewSound(const QString &path);
    void updatePresetButtons();
    int selectedPresetId() const;

    NotificationSoundPlayer *soundPlayer_ = nullptr;
    QTableWidget *presetTable_ = nullptr;
    QPushButton *editPresetButton_ = nullptr;
    QPushButton *copyPresetButton_ = nullptr;
    QPushButton *defaultPresetButton_ = nullptr;
    QPushButton *deletePresetButton_ = nullptr;
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
