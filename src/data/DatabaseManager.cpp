#include "data/DatabaseManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

bool DatabaseManager::initialize()
{
    const QString applicationData =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (applicationData.isEmpty()) {
        lastError_ = QStringLiteral("无法确定应用数据目录。");
        return false;
    }

    QDir directory(applicationData);
    if (!directory.mkpath(QStringLiteral("."))) {
        lastError_ = QStringLiteral("无法创建数据目录：%1").arg(applicationData);
        return false;
    }

    databasePath_ = directory.filePath(QStringLiteral("focusflow.db"));
    if (!applyPendingRestore(applicationData)) {
        return false;
    }

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db.setDatabaseName(databasePath_);
    if (!db.open()) {
        lastError_ = db.lastError().text();
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout = 5000"));

    return createSchema() && seedDefaults();
}

bool DatabaseManager::applyPendingRestore(const QString &applicationDataPath)
{
    QDir directory(applicationDataPath);
    const QString pendingPath =
        directory.filePath(QStringLiteral("focusflow.restore-pending.db"));
    if (!QFileInfo::exists(pendingPath)) {
        return true;
    }

    const QString previousPath =
        directory.filePath(QStringLiteral("focusflow.before-restore.db"));
    QFile::remove(previousPath);

    const bool hadCurrentDatabase = QFileInfo::exists(databasePath_);
    if (hadCurrentDatabase && !QFile::rename(databasePath_, previousPath)) {
        lastError_ = QStringLiteral("无法暂存当前数据库，恢复操作已取消。");
        return false;
    }

    if (!QFile::rename(pendingPath, databasePath_)) {
        if (hadCurrentDatabase) {
            QFile::rename(previousPath, databasePath_);
        }
        lastError_ = QStringLiteral("无法应用待恢复的数据库，原数据未被替换。");
        return false;
    }

    // WAL/SHM 文件属于被替换的旧数据库，不能让它们作用于恢复后的文件。
    QFile::remove(databasePath_ + QStringLiteral("-wal"));
    QFile::remove(databasePath_ + QStringLiteral("-shm"));
    QFile::remove(previousPath);
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(connectionName_);
}

QString DatabaseManager::databasePath() const
{
    return databasePath_;
}

QString DatabaseManager::lastError() const
{
    return lastError_;
}

bool DatabaseManager::createSchema()
{
    const QStringList statements{
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS categories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                color TEXT NOT NULL DEFAULT '#4F6EF7',
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            )
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS projects (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                description TEXT NOT NULL DEFAULT '',
                color TEXT NOT NULL DEFAULT '#4F6EF7',
                archived INTEGER NOT NULL DEFAULT 0,
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            )
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS tasks (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                title TEXT NOT NULL,
                description TEXT NOT NULL DEFAULT '',
                project_id INTEGER,
                category_id INTEGER,
                timer_preset_id INTEGER,
                importance INTEGER NOT NULL DEFAULT 3 CHECK (importance BETWEEN 1 AND 5),
                due_at TEXT,
                estimated_minutes INTEGER NOT NULL DEFAULT 25 CHECK (estimated_minutes >= 0),
                status TEXT NOT NULL DEFAULT 'pending'
                    CHECK (status IN ('pending', 'in_progress', 'completed', 'cancelled')),
                is_deleted INTEGER NOT NULL DEFAULT 0,
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                completed_at TEXT,
                FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE SET NULL,
                FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL,
                FOREIGN KEY (timer_preset_id) REFERENCES timer_presets(id) ON DELETE SET NULL
            )
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS focus_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                task_id INTEGER,
                session_type TEXT NOT NULL DEFAULT 'focus',
                status TEXT NOT NULL DEFAULT 'completed',
                start_time TEXT NOT NULL,
                end_time TEXT,
                planned_seconds INTEGER NOT NULL DEFAULT 0,
                actual_seconds INTEGER NOT NULL DEFAULT 0,
                interruption_reason TEXT NOT NULL DEFAULT '',
                FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE SET NULL
            )
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS timer_presets (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                focus_minutes INTEGER NOT NULL DEFAULT 25,
                short_break_minutes INTEGER NOT NULL DEFAULT 5,
                long_break_minutes INTEGER NOT NULL DEFAULT 15,
                cycles_before_long_break INTEGER NOT NULL DEFAULT 4,
                breaks_enabled INTEGER NOT NULL DEFAULT 1,
                auto_start_break INTEGER NOT NULL DEFAULT 0,
                auto_start_focus INTEGER NOT NULL DEFAULT 0,
                auto_start_next_focus INTEGER NOT NULL DEFAULT 0,
                is_default INTEGER NOT NULL DEFAULT 0,
                is_builtin INTEGER NOT NULL DEFAULT 0
            )
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS settings (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
        )"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tasks_due_at ON tasks(due_at)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_focus_start_time ON focus_sessions(start_time)"),
    };

    auto db = database();
    if (!db.transaction()) {
        lastError_ = db.lastError().text();
        return false;
    }

    for (const auto &statement : statements) {
        if (!executeStatement(statement)) {
            db.rollback();
            return false;
        }
    }

    bool hasTimerPresetColumn = false;
    QSqlQuery columns(db);
    if (!columns.exec(QStringLiteral("PRAGMA table_info(tasks)"))) {
        lastError_ = columns.lastError().text();
        db.rollback();
        return false;
    }
    while (columns.next()) {
        if (columns.value(QStringLiteral("name")).toString()
            == QStringLiteral("timer_preset_id")) {
            hasTimerPresetColumn = true;
            break;
        }
    }
    if (!hasTimerPresetColumn
        && !executeStatement(QStringLiteral(
            "ALTER TABLE tasks ADD COLUMN timer_preset_id INTEGER "
            "REFERENCES timer_presets(id) ON DELETE SET NULL"))) {
        db.rollback();
        return false;
    }

    bool hasBuiltInPresetColumn = false;
    bool hasBreaksEnabledColumn = false;
    bool hasAutoNextFocusColumn = false;
    QSqlQuery presetColumns(db);
    if (!presetColumns.exec(QStringLiteral(
            "PRAGMA table_info(timer_presets)"))) {
        lastError_ = presetColumns.lastError().text();
        db.rollback();
        return false;
    }
    while (presetColumns.next()) {
        const QString columnName =
            presetColumns.value(QStringLiteral("name")).toString();
        if (columnName == QStringLiteral("is_builtin")) {
            hasBuiltInPresetColumn = true;
        } else if (columnName == QStringLiteral("breaks_enabled")) {
            hasBreaksEnabledColumn = true;
        } else if (columnName == QStringLiteral("auto_start_next_focus")) {
            hasAutoNextFocusColumn = true;
        }
    }
    if (!hasBuiltInPresetColumn
        && !executeStatement(QStringLiteral(
            "ALTER TABLE timer_presets ADD COLUMN is_builtin INTEGER "
            "NOT NULL DEFAULT 0"))) {
        db.rollback();
        return false;
    }
    if (!hasBreaksEnabledColumn
        && !executeStatement(QStringLiteral(
            "ALTER TABLE timer_presets ADD COLUMN breaks_enabled INTEGER "
            "NOT NULL DEFAULT 1"))) {
        db.rollback();
        return false;
    }
    if (!hasAutoNextFocusColumn
        && !executeStatement(QStringLiteral(
            "ALTER TABLE timer_presets ADD COLUMN auto_start_next_focus INTEGER "
            "NOT NULL DEFAULT 0"))) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        lastError_ = db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::executeStatement(const QString &statement)
{
    QSqlQuery query(database());
    if (!query.exec(statement)) {
        lastError_ = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::seedDefaults()
{
    auto db = database();
    if (!db.transaction()) {
        lastError_ = db.lastError().text();
        return false;
    }

    QSqlQuery category(db);
    category.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO categories(name, color) VALUES(?, ?)"));
    const QList<QPair<QString, QString>> categories{
        {QStringLiteral("工作"), QStringLiteral("#4F6EF7")},
        {QStringLiteral("学习"), QStringLiteral("#8B5CF6")},
        {QStringLiteral("生活"), QStringLiteral("#14B8A6")},
    };
    for (const auto &[name, color] : categories) {
        category.addBindValue(name);
        category.addBindValue(color);
        if (!category.exec()) {
            lastError_ = category.lastError().text();
            db.rollback();
            return false;
        }
    }

    QSqlQuery project(db);
    project.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO projects(name, description, color) VALUES(?, ?, ?)"));
    project.addBindValue(QStringLiteral("个人事务"));
    project.addBindValue(QStringLiteral("默认的个人任务项目"));
    project.addBindValue(QStringLiteral("#4F6EF7"));
    if (!project.exec()) {
        lastError_ = project.lastError().text();
        db.rollback();
        return false;
    }

    struct PresetSeed {
        QString name;
        int focusMinutes;
        int shortBreakMinutes;
        int longBreakMinutes;
        int cycles;
        bool isDefault;
    };
    const QList<PresetSeed> presets{
        {QStringLiteral("经典番茄钟"), 25, 5, 15, 4, true},
        {QStringLiteral("快速处理"), 15, 3, 10, 4, false},
        {QStringLiteral("深度工作"), 50, 10, 20, 3, false},
        {QStringLiteral("长时专注"), 90, 15, 30, 2, false},
    };
    QSqlQuery preset(db);
    preset.prepare(QStringLiteral(R"(
        INSERT OR IGNORE INTO timer_presets(
            name, focus_minutes, short_break_minutes,
            long_break_minutes, cycles_before_long_break, is_default,
            is_builtin
        ) VALUES(?, ?, ?, ?, ?, ?, 1)
    )"));
    for (const auto &item : presets) {
        preset.addBindValue(item.name);
        preset.addBindValue(item.focusMinutes);
        preset.addBindValue(item.shortBreakMinutes);
        preset.addBindValue(item.longBreakMinutes);
        preset.addBindValue(item.cycles);
        preset.addBindValue(item.isDefault ? 1 : 0);
        if (!preset.exec()) {
            lastError_ = preset.lastError().text();
            db.rollback();
            return false;
        }
    }

    QSqlQuery markBuiltIn(db);
    if (!markBuiltIn.exec(QStringLiteral(R"(
        UPDATE timer_presets SET is_builtin = 1
        WHERE name IN ('经典番茄钟', '快速处理', '深度工作', '长时专注')
    )"))) {
        lastError_ = markBuiltIn.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery ensureDefault(db);
    if (!ensureDefault.exec(QStringLiteral(R"(
        UPDATE timer_presets SET is_default = 1
        WHERE id = (SELECT id FROM timer_presets ORDER BY id LIMIT 1)
          AND NOT EXISTS (SELECT 1 FROM timer_presets WHERE is_default = 1)
    )"))) {
        lastError_ = ensureDefault.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        lastError_ = db.lastError().text();
        return false;
    }
    return true;
}
