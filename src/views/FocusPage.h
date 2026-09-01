#pragma once

#include "models/TimerSettings.h"
#include "services/FocusTimer.h"

#include <QWidget>

class FocusRepository;
class NotificationSoundPlayer;
class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;

class FocusPage final : public QWidget
{
    Q_OBJECT

public:
    explicit FocusPage(QWidget *parent = nullptr);

signals:
    void notificationRequested(const QString &title, const QString &message);
    void focusDataChanged();

public slots:
    void reloadSettings();
    void refreshTasks();

private slots:
    void startCurrentPhase();
    void togglePause();
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

private:
    void buildInterface();
    FocusTimer::Phase selectedPhase() const;
    int durationSeconds(FocusTimer::Phase phase) const;
    void selectPhase(FocusTimer::Phase phase);
    void playCompletionSound(FocusTimer::Phase phase);
    static QString formatSeconds(int totalSeconds);
    static QString phaseText(FocusTimer::Phase phase);

    FocusTimer timer_;
    TimerSettings settings_;
    NotificationSoundPlayer *soundPlayer_ = nullptr;
    QComboBox *taskCombo_ = nullptr;
    QComboBox *phaseCombo_ = nullptr;
    QLabel *timerLabel_ = nullptr;
    QLabel *phaseLabel_ = nullptr;
    QLabel *cycleLabel_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QPushButton *startButton_ = nullptr;
    QPushButton *pauseButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    int completedFocusCycles_ = 0;
    int currentTaskId_ = -1;
};
