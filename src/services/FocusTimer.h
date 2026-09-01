#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>

class FocusTimer final : public QObject
{
    Q_OBJECT

public:
    enum class Phase {
        Focus,
        ShortBreak,
        LongBreak,
    };
    Q_ENUM(Phase)

    enum class State {
        Idle,
        Running,
        Paused,
    };
    Q_ENUM(State)

    explicit FocusTimer(QObject *parent = nullptr);

    void start(Phase phase, int durationSeconds, int taskId = -1);
    void pause();
    void resume();
    void stopEarly();

    Phase phase() const;
    State state() const;
    int plannedSeconds() const;
    int remainingSeconds() const;
    int actualSeconds() const;

signals:
    void timeChanged(int remainingSeconds, int plannedSeconds);
    void stateChanged(FocusTimer::State state);
    void sessionEnded(FocusTimer::Phase phase,
                      bool completed,
                      int taskId,
                      QDateTime startedAt,
                      QDateTime endedAt,
                      int plannedSeconds,
                      int actualSeconds);

private slots:
    void updateRemainingTime();

private:
    void finish(bool completed);

    QTimer tickTimer_;
    Phase phase_ = Phase::Focus;
    State state_ = State::Idle;
    int taskId_ = -1;
    int plannedSeconds_ = 0;
    int remainingSeconds_ = 0;
    QDateTime startedAt_;
    QDateTime deadline_;
};
