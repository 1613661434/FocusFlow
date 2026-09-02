#pragma once

#include "models/TimerPreset.h"
#include "models/TimerSettings.h"
#include "services/FocusTimer.h"

#include <QTimer>
#include <QVector>
#include <QWidget>

class FocusRepository;
class NotificationSoundPlayer;
class QComboBox;
class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QEvent;

class FocusPage final : public QWidget
{
    Q_OBJECT

public:
    explicit FocusPage(QWidget *parent = nullptr);

signals:
    void notificationRequested(const QString &title, const QString &message);
    void focusDataChanged();
    void tasksChanged();
    void presetsChanged();
    void trayStatusChanged(const QString &status);

public slots:
    void reloadSettings();
    void refreshTasks();
    void selectTask(int taskId);

private slots:
    void handlePrimaryAction();
    void startCurrentPhase();
    void stopEarly();
    void updateTime(int remainingSeconds, int plannedSeconds);
    void updateState(FocusTimer::State state);
    void handleSessionEnded(FocusTimer::Phase phase,
                            bool completed,
                            int taskId,
                            QDateTime startedAt,
                            QDateTime endedAt,
                            int plannedSeconds,
                            int actualSeconds);
    void updateIdleDuration();
    void handlePresetChanged();
    void applyTaskPreset();
    void setSelectedPresetAsTaskDefault();
    void confirmCustomMinutes();
    void saveCustomPreset();
    void restoreIdleStatus();

private:
    void buildInterface();
    void reloadTaskFilters();
    void reloadPresets();
    void refreshFilteredTasks();
    TimerPreset selectedPreset() const;
    void updatePresetControls();
    void updateTrayStatus();
    TimerPreset customEditorPreset() const;
    void loadCustomEditor(const TimerPreset &preset);
    bool customEditorDirty() const;
    void clearCustomEditorFocus();
    void showTemporaryStatus(const QString &message, int durationMs = 2500);
    FocusTimer::Phase selectedPhase() const;
    int durationSeconds(FocusTimer::Phase phase) const;
    void selectPhase(FocusTimer::Phase phase);
    void playCompletionSound(FocusTimer::Phase phase);
    static QString formatSeconds(int totalSeconds);
    static QString phaseText(FocusTimer::Phase phase);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    FocusTimer timer_;
    QTimer statusResetTimer_;
    TimerSettings settings_;
    NotificationSoundPlayer *soundPlayer_ = nullptr;
    QComboBox *projectFilter_ = nullptr;
    QComboBox *categoryFilter_ = nullptr;
    QComboBox *taskCombo_ = nullptr;
    QComboBox *presetCombo_ = nullptr;
    QComboBox *phaseCombo_ = nullptr;
    QLabel *customMinutesLabel_ = nullptr;
    QWidget *customMinutesRow_ = nullptr;
    QSpinBox *customMinutes_ = nullptr;
    QSpinBox *customShortBreakMinutes_ = nullptr;
    QSpinBox *customLongBreakMinutes_ = nullptr;
    QSpinBox *customCycles_ = nullptr;
    QCheckBox *customAutoStartBreak_ = nullptr;
    QCheckBox *customAutoStartFocus_ = nullptr;
    QPushButton *confirmCustomMinutesButton_ = nullptr;
    QPushButton *saveCustomPresetButton_ = nullptr;
    QLabel *timerLabel_ = nullptr;
    QLabel *phaseLabel_ = nullptr;
    QLabel *cycleLabel_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QPushButton *primaryActionButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QPushButton *setTaskPresetButton_ = nullptr;
    QVector<TimerPreset> presets_;
    TimerPreset defaultPreset_;
    TimerPreset confirmedCustomPreset_;
    int completedFocusCycles_ = 0;
    int currentTaskId_ = -1;
    int currentRemainingSeconds_ = 0;
    QString currentTaskTitle_;
    bool completeTaskWhenSessionEnds_ = false;
};
