#include "repositories/AnalyticsRepository.h"

#include "data/DatabaseManager.h"

#include <QDate>
#include <QSqlQuery>

DashboardMetrics AnalyticsRepository::dashboardMetrics() const
{
    DashboardMetrics metrics;
    metrics.pendingTasks = scalar(QStringLiteral(R"(
        SELECT COUNT(*) FROM tasks
        WHERE is_deleted = 0 AND status NOT IN ('completed', 'cancelled')
    )"));
    metrics.dueToday = scalar(QStringLiteral(R"(
        SELECT COUNT(*) FROM tasks
        WHERE is_deleted = 0 AND status NOT IN ('completed', 'cancelled')
          AND date(due_at) = date('now', 'localtime')
    )"));
    metrics.overdueTasks = scalar(QStringLiteral(R"(
        SELECT COUNT(*) FROM tasks
        WHERE is_deleted = 0 AND status NOT IN ('completed', 'cancelled')
          AND datetime(due_at) < datetime('now', 'localtime')
    )"));
    metrics.completedToday = scalar(QStringLiteral(R"(
        SELECT COUNT(*) FROM tasks
        WHERE is_deleted = 0 AND status = 'completed'
          AND date(completed_at) = date('now', 'localtime')
    )"));
    metrics.focusSecondsToday = scalar(QStringLiteral(R"(
        SELECT COALESCE(SUM(actual_seconds), 0) FROM focus_sessions
        WHERE session_type = 'focus'
          AND status IN ('completed', 'interrupted')
          AND actual_seconds > 0
          AND date(start_time) = date('now', 'localtime')
    )"));
    metrics.focusSecondsLastSevenDays = scalar(QStringLiteral(R"(
        SELECT COALESCE(SUM(actual_seconds), 0) FROM focus_sessions
        WHERE session_type = 'focus'
          AND status IN ('completed', 'interrupted')
          AND actual_seconds > 0
          AND date(start_time) >= date('now', 'localtime', '-6 days')
    )"));
    return metrics;
}

QVector<DailyProductivity> AnalyticsRepository::lastSevenDays() const
{
    QVector<DailyProductivity> result;
    auto database = DatabaseManager::instance().database();
    const QDate today = QDate::currentDate();

    QSqlQuery focusQuery(database);
    focusQuery.prepare(QStringLiteral(R"(
        SELECT COALESCE(SUM(actual_seconds), 0)
        FROM focus_sessions
        WHERE session_type = 'focus'
          AND status IN ('completed', 'interrupted')
          AND actual_seconds > 0
          AND date(start_time) = :date
    )"));
    QSqlQuery taskQuery(database);
    taskQuery.prepare(QStringLiteral(R"(
        SELECT COUNT(*) FROM tasks
        WHERE is_deleted = 0 AND status = 'completed'
          AND date(completed_at) = :date
    )"));

    for (int offset = 6; offset >= 0; --offset) {
        const QDate date = today.addDays(-offset);
        const QString dateText = date.toString(Qt::ISODate);
        int focusSeconds = 0;
        int completed = 0;

        focusQuery.bindValue(QStringLiteral(":date"), dateText);
        if (focusQuery.exec() && focusQuery.next()) {
            focusSeconds = focusQuery.value(0).toInt();
        }
        taskQuery.bindValue(QStringLiteral(":date"), dateText);
        if (taskQuery.exec() && taskQuery.next()) {
            completed = taskQuery.value(0).toInt();
        }
        result.push_back({date.toString(QStringLiteral("MM-dd")),
                          focusSeconds > 0 ? (focusSeconds + 59) / 60 : 0,
                          completed});
    }
    return result;
}

QVector<CategoryFocus> AnalyticsRepository::focusByCategory() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QStringLiteral(R"(
        SELECT COALESCE(c.name, '未分类') AS category_name,
               CASE WHEN COALESCE(SUM(fs.actual_seconds), 0) > 0
                    THEN (SUM(fs.actual_seconds) + 59) / 60
                    ELSE 0 END AS focus_minutes
        FROM focus_sessions fs
        LEFT JOIN tasks t ON t.id = fs.task_id
        LEFT JOIN categories c ON c.id = t.category_id
        WHERE fs.session_type = 'focus'
          AND fs.status IN ('completed', 'interrupted')
          AND fs.actual_seconds > 0
          AND date(fs.start_time) >= date('now', 'localtime', '-29 days')
        GROUP BY COALESCE(c.name, '未分类')
        HAVING focus_minutes > 0
        ORDER BY focus_minutes DESC
    )"));

    QVector<CategoryFocus> result;
    while (query.next()) {
        result.push_back({query.value(0).toString(), query.value(1).toInt()});
    }
    return result;
}

int AnalyticsRepository::scalar(const QString &sql) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (query.exec(sql) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
