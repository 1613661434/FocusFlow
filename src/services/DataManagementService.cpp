#include "services/DataManagementService.h"

#include "data/DatabaseManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>
#include <QUuid>

namespace {
QString escapedSqlString(QString value)
{
    return value.replace(QLatin1Char('\''), QStringLiteral("''"));
}

bool prepareDestination(const QString &destinationPath, QString *errorMessage)
{
    const QFileInfo destination(destinationPath);
    if (destinationPath.trimmed().isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("没有选择目标文件。");
        }
        return false;
    }

    QDir directory = destination.absoluteDir();
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法创建目标目录：%1").arg(directory.path());
        }
        return false;
    }
    return true;
}
}

bool DataManagementService::backupDatabase(const QString &destinationPath,
                                           QString *errorMessage) const
{
    if (!prepareDestination(destinationPath, errorMessage)) {
        return false;
    }

    const QString currentPath = DatabaseManager::instance().databasePath();
    if (QFileInfo(destinationPath).absoluteFilePath()
        == QFileInfo(currentPath).absoluteFilePath()) {
        assignError(QStringLiteral("备份位置不能是当前正在使用的数据库。"), errorMessage);
        return false;
    }

    if (QFileInfo::exists(destinationPath) && !QFile::remove(destinationPath)) {
        assignError(QStringLiteral("无法覆盖已有备份文件。"), errorMessage);
        return false;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    const QString statement = QStringLiteral("VACUUM INTO '%1'")
                                  .arg(escapedSqlString(
                                      QFileInfo(destinationPath).absoluteFilePath()));
    if (!query.exec(statement)) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    return QFileInfo::exists(destinationPath);
}

bool DataManagementService::prepareRestore(const QString &sourcePath,
                                           QString *recoveryBackupPath,
                                           QString *errorMessage) const
{
    if (!validateBackup(sourcePath, errorMessage)) {
        return false;
    }

    const QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDirectory(dataPath);
    QDir backupDirectory(dataDirectory.filePath(QStringLiteral("backups")));
    if (!backupDirectory.mkpath(QStringLiteral("."))) {
        assignError(QStringLiteral("无法创建自动备份目录。"), errorMessage);
        return false;
    }

    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString recoveryPath = backupDirectory.filePath(
        QStringLiteral("before-restore-%1.db").arg(timestamp));
    if (!backupDatabase(recoveryPath, errorMessage)) {
        return false;
    }

    const QString pendingPath =
        dataDirectory.filePath(QStringLiteral("focusflow.restore-pending.db"));
    QFile::remove(pendingPath);
    if (!QFile::copy(sourcePath, pendingPath)) {
        assignError(QStringLiteral("无法把备份文件复制到应用数据目录。"), errorMessage);
        return false;
    }

    if (recoveryBackupPath != nullptr) {
        *recoveryBackupPath = recoveryPath;
    }
    return true;
}

bool DataManagementService::exportTasksCsv(const QString &destinationPath,
                                           QString *errorMessage) const
{
    if (!prepareDestination(destinationPath, errorMessage)) {
        return false;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec(QStringLiteral(R"(
        SELECT t.id, t.title, t.description, p.name, c.name, t.importance,
               t.due_at, t.estimated_minutes, t.status, t.created_at, t.completed_at
        FROM tasks t
        LEFT JOIN projects p ON p.id = t.project_id
        LEFT JOIN categories c ON c.id = t.category_id
        WHERE t.is_deleted = 0
        ORDER BY t.id
    )"))) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }

    QSaveFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        assignError(file.errorString(), errorMessage);
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << QChar(0xFEFF)
           << QStringLiteral("编号,标题,描述,项目,分类,重要程度,截止时间,预计分钟,状态,创建时间,完成时间\n");
    while (query.next()) {
        QStringList cells;
        for (int column = 0; column < 11; ++column) {
            const QString value = column == 8
                ? statusLabel(query.value(column).toString())
                : query.value(column).toString();
            cells << csvCell(value);
        }
        stream << cells.join(QLatin1Char(',')) << QLatin1Char('\n');
    }
    if (!file.commit()) {
        assignError(file.errorString(), errorMessage);
        return false;
    }
    return true;
}

bool DataManagementService::exportFocusSessionsCsv(const QString &destinationPath,
                                                   QString *errorMessage) const
{
    if (!prepareDestination(destinationPath, errorMessage)) {
        return false;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec(QStringLiteral(R"(
        SELECT f.id, COALESCE(t.title, ''), f.session_type, f.status,
               f.start_time, f.end_time, f.planned_seconds, f.actual_seconds,
               f.interruption_reason
        FROM focus_sessions f
        LEFT JOIN tasks t ON t.id = f.task_id
        ORDER BY f.start_time
    )"))) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }

    QSaveFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        assignError(file.errorString(), errorMessage);
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << QChar(0xFEFF)
           << QStringLiteral("编号,关联任务,类型,状态,开始时间,结束时间,计划秒数,实际秒数,中断原因\n");
    while (query.next()) {
        QStringList cells;
        for (int column = 0; column < 9; ++column) {
            QString value = query.value(column).toString();
            if (column == 2) {
                value = sessionTypeLabel(value);
            } else if (column == 3) {
                value = statusLabel(value);
            }
            cells << csvCell(value);
        }
        stream << cells.join(QLatin1Char(',')) << QLatin1Char('\n');
    }
    if (!file.commit()) {
        assignError(file.errorString(), errorMessage);
        return false;
    }
    return true;
}

bool DataManagementService::clearFocusStatistics(QString *errorMessage) const
{
    QSqlDatabase database = DatabaseManager::instance().database();
    if (!database.transaction()) {
        assignError(database.lastError().text(), errorMessage);
        return false;
    }

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("DELETE FROM focus_sessions"))) {
        database.rollback();
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    if (!database.commit()) {
        assignError(database.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

bool DataManagementService::validateBackup(const QString &sourcePath,
                                           QString *errorMessage) const
{
    const QFileInfo source(sourcePath);
    if (!source.isFile() || !source.isReadable()) {
        assignError(QStringLiteral("备份文件不存在或无法读取。"), errorMessage);
        return false;
    }

    const QString connectionName =
        QStringLiteral("focusflow-backup-check-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QString validationError;
    bool valid = false;
    {
        QSqlDatabase candidate =
            QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        candidate.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        candidate.setDatabaseName(source.absoluteFilePath());
        if (!candidate.open()) {
            validationError = candidate.lastError().text();
        } else {
            QSqlQuery integrity(candidate);
            if (!integrity.exec(QStringLiteral("PRAGMA quick_check"))
                || !integrity.next()
                || integrity.value(0).toString() != QStringLiteral("ok")) {
                validationError = QStringLiteral("SQLite 完整性检查未通过。");
            } else {
                QSqlQuery tables(candidate);
                tables.exec(QStringLiteral(
                    "SELECT name FROM sqlite_master WHERE type = 'table'"));
                QSet<QString> names;
                while (tables.next()) {
                    names.insert(tables.value(0).toString());
                }
                valid = names.contains(QStringLiteral("tasks"))
                    && names.contains(QStringLiteral("focus_sessions"))
                    && names.contains(QStringLiteral("settings"));
                if (!valid) {
                    validationError = QStringLiteral("文件不是有效的 FocusFlow 备份。");
                }
            }
            candidate.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);

    if (!valid) {
        assignError(validationError, errorMessage);
    }
    return valid;
}

QString DataManagementService::csvCell(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}

QString DataManagementService::statusLabel(const QString &status)
{
    if (status == QStringLiteral("pending")) {
        return QStringLiteral("待处理");
    }
    if (status == QStringLiteral("in_progress")) {
        return QStringLiteral("进行中");
    }
    if (status == QStringLiteral("completed")) {
        return QStringLiteral("已完成");
    }
    if (status == QStringLiteral("cancelled")) {
        return QStringLiteral("已取消");
    }
    if (status == QStringLiteral("interrupted")) {
        return QStringLiteral("已中断");
    }
    return status;
}

QString DataManagementService::sessionTypeLabel(const QString &sessionType)
{
    if (sessionType == QStringLiteral("focus")) {
        return QStringLiteral("专注");
    }
    if (sessionType == QStringLiteral("short_break")) {
        return QStringLiteral("短休息");
    }
    if (sessionType == QStringLiteral("long_break")) {
        return QStringLiteral("长休息");
    }
    return sessionType;
}

void DataManagementService::assignError(const QString &message,
                                        QString *destination)
{
    if (destination != nullptr) {
        *destination = message;
    }
}
