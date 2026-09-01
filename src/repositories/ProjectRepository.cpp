#include "repositories/ProjectRepository.h"

#include "data/DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>

QVector<Project> ProjectRepository::projects(bool includeArchived) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    QString sql = QStringLiteral(
        "SELECT id, name, description, color, archived FROM projects");
    if (!includeArchived) {
        sql += QStringLiteral(" WHERE archived = 0");
    }
    sql += QStringLiteral(" ORDER BY archived, name");
    query.exec(sql);

    QVector<Project> result;
    while (query.next()) {
        result.push_back({query.value(0).toInt(),
                          query.value(1).toString(),
                          query.value(2).toString(),
                          query.value(3).toString(),
                          query.value(4).toBool()});
    }
    return result;
}

QVector<LookupItem> ProjectRepository::categories() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QStringLiteral("SELECT id, name, color FROM categories ORDER BY name"));

    QVector<LookupItem> result;
    while (query.next()) {
        result.push_back({query.value(0).toInt(),
                          query.value(1).toString(),
                          query.value(2).toString()});
    }
    return result;
}

bool ProjectRepository::saveProject(Project &project, QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (project.id < 0) {
        query.prepare(QStringLiteral(R"(
            INSERT INTO projects(name, description, color, archived)
            VALUES(:name, :description, :color, :archived)
        )"));
    } else {
        query.prepare(QStringLiteral(R"(
            UPDATE projects SET
                name = :name,
                description = :description,
                color = :color,
                archived = :archived
            WHERE id = :id
        )"));
        query.bindValue(QStringLiteral(":id"), project.id);
    }
    query.bindValue(QStringLiteral(":name"), project.name.trimmed());
    query.bindValue(QStringLiteral(":description"), project.description.trimmed());
    query.bindValue(QStringLiteral(":color"), project.color);
    query.bindValue(QStringLiteral(":archived"), project.archived ? 1 : 0);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    if (project.id < 0) {
        project.id = query.lastInsertId().toInt();
    }
    return true;
}

bool ProjectRepository::setProjectArchived(int id,
                                           bool archived,
                                           QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("UPDATE projects SET archived = :archived WHERE id = :id"));
    query.bindValue(QStringLiteral(":archived"), archived ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

bool ProjectRepository::deleteProject(int id, QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("DELETE FROM projects WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

bool ProjectRepository::saveCategory(LookupItem &category, QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (category.id < 0) {
        query.prepare(QStringLiteral(
            "INSERT INTO categories(name, color) VALUES(:name, :color)"));
    } else {
        query.prepare(QStringLiteral(
            "UPDATE categories SET name = :name, color = :color WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), category.id);
    }
    query.bindValue(QStringLiteral(":name"), category.name.trimmed());
    query.bindValue(QStringLiteral(":color"), category.color);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    if (category.id < 0) {
        category.id = query.lastInsertId().toInt();
    }
    return true;
}

bool ProjectRepository::deleteCategory(int id, QString *errorMessage) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("DELETE FROM categories WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

void ProjectRepository::assignError(const QString &message, QString *destination)
{
    if (destination != nullptr) {
        *destination = message;
    }
}
