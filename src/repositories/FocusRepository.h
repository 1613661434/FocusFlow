#pragma once

#include "services/FocusTimer.h"

#include <QDateTime>
#include <QString>

class FocusRepository final
{
public:
    bool recordSession(int taskId,
                       FocusTimer::Phase phase,
                       bool completed,
                       const QDateTime &startedAt,
                       const QDateTime &endedAt,
                       int plannedSeconds,
                       int actualSeconds,
                       const QString &interruptionReason = {},
                       QString *errorMessage = nullptr) const;
};
