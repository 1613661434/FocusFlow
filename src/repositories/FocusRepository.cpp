#include "repositories/FocusRepository.h"

#include "data/DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString phaseName(FocusTimer::Phase phase)
{
    switch (phase) {
    case FocusTimer::Phase::ShortBreak:
        return QStringLiteral("short_break");
    case FocusTimer::Phase::LongBreak:
        return QStringLiteral("long_break");
    case FocusTimer::Phase::Focus:
    default:
        return QStringLiteral("focus");
    }
}
}

bool FocusRepository::recordSession(int taskId,
                                    FocusTimer::Phase phase,
                                    bool completed,
                                    const QDateTime &startedAt,
                                    const QDateTime &endedAt,
                                    int plannedSeconds,
                                    int actualSeconds,
                                    const QString &interruptionReason,
                                    QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral(R"(
        INSERT INTO focus_sessions(
            task_id, session_type, status, start_time, end_time,
            planned_seconds, actual_seconds, interruption_reason
        ) VALUES(
            :task_id, :session_type, :status, :start_time, :end_time,
            :planned_seconds, :actual_seconds, :interruption_reason
        )
    )"));
    query.bindValue(QStringLiteral(":task_id"),
                    taskId > 0 ? QVariant(taskId) : QVariant());
    query.bindValue(QStringLiteral(":session_type"), phaseName(phase));
    query.bindValue(QStringLiteral(":status"),
                    completed ? QStringLiteral("completed") : QStringLiteral("interrupted"));
    query.bindValue(QStringLiteral(":start_time"), startedAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":end_time"), endedAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":planned_seconds"), plannedSeconds);
    query.bindValue(QStringLiteral(":actual_seconds"), actualSeconds);
    query.bindValue(QStringLiteral(":interruption_reason"),
                    interruptionReason.isNull()
                        ? QStringLiteral("")
                        : interruptionReason);

    if (!query.exec()) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }
    return true;
}
