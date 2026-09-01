#include "services/FocusTimer.h"

#include <QtGlobal>

FocusTimer::FocusTimer(QObject *parent)
    : QObject(parent)
{
    tickTimer_.setInterval(200);
    tickTimer_.setTimerType(Qt::PreciseTimer);
    connect(&tickTimer_, &QTimer::timeout,
            this, &FocusTimer::updateRemainingTime);
}

void FocusTimer::start(Phase phase, int durationSeconds, int taskId)
{
    if (durationSeconds <= 0) {
        return;
    }

    phase_ = phase;
    taskId_ = taskId;
    plannedSeconds_ = durationSeconds;
    remainingSeconds_ = durationSeconds;
    startedAt_ = QDateTime::currentDateTime();
    deadline_ = startedAt_.addSecs(durationSeconds);
    state_ = State::Running;
    tickTimer_.start();
    emit stateChanged(state_);
    emit timeChanged(remainingSeconds_, plannedSeconds_);
}

void FocusTimer::pause()
{
    if (state_ != State::Running) {
        return;
    }
    updateRemainingTime();
    tickTimer_.stop();
    state_ = State::Paused;
    emit stateChanged(state_);
}

void FocusTimer::resume()
{
    if (state_ != State::Paused || remainingSeconds_ <= 0) {
        return;
    }
    deadline_ = QDateTime::currentDateTime().addSecs(remainingSeconds_);
    state_ = State::Running;
    tickTimer_.start();
    emit stateChanged(state_);
}

void FocusTimer::stopEarly()
{
    if (state_ == State::Idle) {
        return;
    }
    if (state_ == State::Running) {
        updateRemainingTime();
    }
    finish(false);
}

FocusTimer::Phase FocusTimer::phase() const
{
    return phase_;
}

FocusTimer::State FocusTimer::state() const
{
    return state_;
}

int FocusTimer::plannedSeconds() const
{
    return plannedSeconds_;
}

int FocusTimer::remainingSeconds() const
{
    return remainingSeconds_;
}

int FocusTimer::actualSeconds() const
{
    return qBound(0, plannedSeconds_ - remainingSeconds_, plannedSeconds_);
}

void FocusTimer::updateRemainingTime()
{
    if (state_ != State::Running) {
        return;
    }

    const qint64 milliseconds = QDateTime::currentDateTime().msecsTo(deadline_);
    const int seconds = milliseconds > 0
        ? static_cast<int>((milliseconds + 999) / 1000)
        : 0;
    if (seconds != remainingSeconds_) {
        remainingSeconds_ = seconds;
        emit timeChanged(remainingSeconds_, plannedSeconds_);
    }

    if (remainingSeconds_ <= 0) {
        finish(true);
    }
}

void FocusTimer::finish(bool completed)
{
    tickTimer_.stop();
    const QDateTime endedAt = QDateTime::currentDateTime();
    const int actual = completed ? plannedSeconds_ : actualSeconds();
    const auto endedPhase = phase_;
    const int endedTask = taskId_;
    const int endedPlanned = plannedSeconds_;
    const QDateTime endedStartedAt = startedAt_;

    remainingSeconds_ = completed ? 0 : remainingSeconds_;
    state_ = State::Idle;
    emit timeChanged(remainingSeconds_, plannedSeconds_);
    emit stateChanged(state_);
    emit sessionEnded(endedPhase,
                      completed,
                      endedTask,
                      endedStartedAt,
                      endedAt,
                      endedPlanned,
                      actual);
}
