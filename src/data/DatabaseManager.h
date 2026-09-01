#pragma once

#include <QSqlDatabase>
#include <QString>

class DatabaseManager final
{
public:
    static DatabaseManager &instance();

    bool initialize();
    QSqlDatabase database() const;
    QString databasePath() const;
    QString lastError() const;

    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

private:
    DatabaseManager() = default;

    bool createSchema();
    bool executeStatement(const QString &statement);
    bool seedDefaults();

    QString connectionName_ = QStringLiteral("focusflow-main");
    QString databasePath_;
    QString lastError_;
};
