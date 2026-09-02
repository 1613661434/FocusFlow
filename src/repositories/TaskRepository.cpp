#include "repositories/TaskRepository.h"

#include "data/DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString dateTimeToStorage(const QDateTime &dateTime)
{
    return dateTime.isValid()
        ? dateTime.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"))
        : QString();
}

QDateTime dateTimeFromStorage(const QVariant &value)
{
    if (value.isNull() || value.toString().isEmpty()) {
        return {};
    }
    return QDateTime::fromString(value.toString(), QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
}
}

QVector<Task> TaskRepository::findAll(Filter filter, const QString &searchText) const
{
    QStringList conditions{QStringLiteral("t.is_deleted = 0")};
    switch (filter) {
    case Filter::Recommended:
        conditions << QStringLiteral("t.status NOT IN ('completed', 'cancelled')");
        break;
    case Filter::Today:
        conditions << QStringLiteral("t.status <> 'completed'")
                   << QStringLiteral("date(t.due_at) = date('now', 'localtime')");
        break;
    case Filter::ThisWeek:
        conditions << QStringLiteral("t.status <> 'completed'")
                   << QStringLiteral(
                          "date(t.due_at) BETWEEN date('now', 'localtime') "
                          "AND date('now', 'localtime', '+7 days')");
        break;
    case Filter::Overdue:
        conditions << QStringLiteral("t.status <> 'completed'")
                   << QStringLiteral("datetime(t.due_at) < datetime('now', 'localtime')");
        break;
    case Filter::Completed:
        conditions << QStringLiteral("t.status = 'completed'");
        break;
    case Filter::All:
        break;
    }

    const QString trimmedSearch = searchText.trimmed();
    if (!trimmedSearch.isEmpty()) {
        conditions << QStringLiteral(
            "(t.title LIKE :search OR t.description LIKE :search "
            "OR p.name LIKE :search OR c.name LIKE :search)");
    }

    const QString sql = QStringLiteral(R"(
        SELECT t.id, t.title, t.description, t.project_id, p.name AS project_name,
               t.category_id, c.name AS category_name, t.importance, t.due_at,
               t.estimated_minutes, t.status, t.created_at, t.completed_at,
               p.color AS project_color, c.color AS category_color,
               t.timer_preset_id, tp.name AS timer_preset_name
        FROM tasks t
        LEFT JOIN projects p ON p.id = t.project_id
        LEFT JOIN categories c ON c.id = t.category_id
        LEFT JOIN timer_presets tp ON tp.id = t.timer_preset_id
        WHERE %1
        ORDER BY
            CASE WHEN t.status = 'completed' THEN 1 ELSE 0 END,
            CASE WHEN t.due_at IS NULL OR t.due_at = '' THEN 1 ELSE 0 END,
            t.due_at ASC,
            t.importance DESC,
            t.id DESC
    )").arg(conditions.join(QStringLiteral(" AND ")));

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(sql);
    if (!trimmedSearch.isEmpty()) {
        query.bindValue(QStringLiteral(":search"),
                        QStringLiteral("%%1%").arg(trimmedSearch));
    }

    QVector<Task> result;
    if (!query.exec()) {
        return result;
    }
    while (query.next()) {
        result.push_back(fromQuery(query));
    }
    return result;
}

Task TaskRepository::findById(int id) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral(R"(
        SELECT t.id, t.title, t.description, t.project_id, p.name AS project_name,
               t.category_id, c.name AS category_name, t.importance, t.due_at,
               t.estimated_minutes, t.status, t.created_at, t.completed_at,
               p.color AS project_color, c.color AS category_color,
               t.timer_preset_id, tp.name AS timer_preset_name
        FROM tasks t
        LEFT JOIN projects p ON p.id = t.project_id
        LEFT JOIN categories c ON c.id = t.category_id
        LEFT JOIN timer_presets tp ON tp.id = t.timer_preset_id
        WHERE t.id = :id AND t.is_deleted = 0
    )"));
    query.bindValue(QStringLiteral(":id"), id);
    if (query.exec() && query.next()) {
        return fromQuery(query);
    }
    return {};
}

QVector<LookupItem> TaskRepository::projects() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QStringLiteral(
        "SELECT id, name, color FROM projects WHERE archived = 0 ORDER BY name"));

    QVector<LookupItem> result;
    while (query.next()) {
        result.push_back({query.value(0).toInt(),
                          query.value(1).toString(),
                          query.value(2).toString()});
    }
    return result;
}

QVector<LookupItem> TaskRepository::categories() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QStringLiteral("SELECT id, name, color FROM categories ORDER BY id"));

    QVector<LookupItem> result;
    while (query.next()) {
        result.push_back({query.value(0).toInt(),
                          query.value(1).toString(),
                          query.value(2).toString()});
    }
    return result;
}

bool TaskRepository::save(Task &task, QString *errorMessage) const
{
    auto db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    if (task.id < 0) {
        query.prepare(QStringLiteral(R"(
            INSERT INTO tasks(
                title, description, project_id, category_id, timer_preset_id,
                importance, due_at, estimated_minutes, status, updated_at
            ) VALUES(
                :title, :description, :project_id, :category_id, :timer_preset_id,
                :importance, :due_at, :estimated_minutes, :status, CURRENT_TIMESTAMP
            )
        )"));
    } else {
        query.prepare(QStringLiteral(R"(
            UPDATE tasks SET
                title = :title,
                description = :description,
                project_id = :project_id,
                category_id = :category_id,
                timer_preset_id = :timer_preset_id,
                importance = :importance,
                due_at = :due_at,
                estimated_minutes = :estimated_minutes,
                status = :status,
                updated_at = CURRENT_TIMESTAMP
            WHERE id = :id AND is_deleted = 0
        )"));
        query.bindValue(QStringLiteral(":id"), task.id);
    }

    query.bindValue(QStringLiteral(":title"), task.title.trimmed());
    query.bindValue(QStringLiteral(":description"),
                    task.description.isNull()
                        ? QStringLiteral("") : task.description.trimmed());
    query.bindValue(QStringLiteral(":project_id"),
                    task.projectId > 0 ? QVariant(task.projectId) : QVariant());
    query.bindValue(QStringLiteral(":category_id"),
                    task.categoryId > 0 ? QVariant(task.categoryId) : QVariant());
    query.bindValue(QStringLiteral(":timer_preset_id"),
                    task.timerPresetId > 0 ? QVariant(task.timerPresetId) : QVariant());
    query.bindValue(QStringLiteral(":importance"), task.importance);
    query.bindValue(QStringLiteral(":due_at"),
                    task.dueAt.isValid() ? QVariant(dateTimeToStorage(task.dueAt)) : QVariant());
    query.bindValue(QStringLiteral(":estimated_minutes"), task.estimatedMinutes);
    query.bindValue(QStringLiteral(":status"), task.status);

    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }

    if (task.id < 0) {
        task.id = query.lastInsertId().toInt();
    }
    return true;
}

bool TaskRepository::setTimerPreset(int id, int timerPresetId,
                                    QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral(R"(
        UPDATE tasks SET
            timer_preset_id = :timer_preset_id,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id AND is_deleted = 0
    )"));
    query.bindValue(QStringLiteral(":timer_preset_id"),
                    timerPresetId > 0 ? QVariant(timerPresetId) : QVariant());
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return query.numRowsAffected() == 1;
}

bool TaskRepository::setCompleted(int id, bool completed, QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral(R"(
        UPDATE tasks SET
            status = :status,
            completed_at = :completed_at,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id AND is_deleted = 0
    )"));
    query.bindValue(QStringLiteral(":status"),
                    completed ? QStringLiteral("completed") : QStringLiteral("pending"));
    query.bindValue(QStringLiteral(":completed_at"),
                    completed
                        ? QVariant(dateTimeToStorage(QDateTime::currentDateTime()))
                        : QVariant());
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

bool TaskRepository::deleteTask(int id, QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("DELETE FROM tasks WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

Task TaskRepository::fromQuery(const QSqlQuery &query)
{
    Task task;
    task.id = query.value(QStringLiteral("id")).toInt();
    task.title = query.value(QStringLiteral("title")).toString();
    task.description = query.value(QStringLiteral("description")).toString();
    task.projectId = query.value(QStringLiteral("project_id")).isNull()
        ? -1 : query.value(QStringLiteral("project_id")).toInt();
    task.projectName = query.value(QStringLiteral("project_name")).toString();
    task.projectColor = query.value(QStringLiteral("project_color")).toString();
    task.categoryId = query.value(QStringLiteral("category_id")).isNull()
        ? -1 : query.value(QStringLiteral("category_id")).toInt();
    task.categoryName = query.value(QStringLiteral("category_name")).toString();
    task.categoryColor = query.value(QStringLiteral("category_color")).toString();
    task.timerPresetId = query.value(QStringLiteral("timer_preset_id")).isNull()
        ? -1 : query.value(QStringLiteral("timer_preset_id")).toInt();
    task.timerPresetName = query.value(QStringLiteral("timer_preset_name")).toString();
    task.importance = query.value(QStringLiteral("importance")).toInt();
    task.dueAt = dateTimeFromStorage(query.value(QStringLiteral("due_at")));
    task.estimatedMinutes = query.value(QStringLiteral("estimated_minutes")).toInt();
    task.status = query.value(QStringLiteral("status")).toString();
    task.createdAt = dateTimeFromStorage(query.value(QStringLiteral("created_at")));
    task.completedAt = dateTimeFromStorage(query.value(QStringLiteral("completed_at")));
    return task;
}

void TaskRepository::assignError(const QString &message, QString *destination)
{
    if (destination != nullptr) {
        *destination = message;
    }
}
