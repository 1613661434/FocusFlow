#include "services/PriorityService.h"

#include <QtGlobal>

int PriorityService::score(const Task &task, const QDateTime &now)
{
    if (task.status == QStringLiteral("completed")) {
        return 0;
    }

    int result = qBound(1, task.importance, 5) * 20;

    if (task.dueAt.isValid()) {
        const qint64 hours = now.secsTo(task.dueAt) / 3600;
        if (hours < 0) {
            const int overdueDays = static_cast<int>((-hours + 23) / 24);
            result += 40 + qMin(overdueDays * 3, 20);
        } else if (hours <= 12) {
            result += 35;
        } else if (hours <= 24) {
            result += 28;
        } else if (hours <= 72) {
            result += 18;
        } else if (hours <= 168) {
            result += 8;
        }
    }

    if (task.estimatedMinutes > 0 && task.estimatedMinutes <= 25) {
        result += 5;
    }
    return result;
}
