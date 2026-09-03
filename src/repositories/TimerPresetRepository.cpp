#include "repositories/TimerPresetRepository.h"

#include "data/DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>

namespace {
const QString kPresetColumns = QStringLiteral(R"(
    id, name, focus_minutes, short_break_minutes, long_break_minutes,
    cycles_before_long_break, breaks_enabled, auto_start_break,
    auto_start_focus, auto_start_next_focus, is_default, is_builtin
)");
}

QVector<TimerPreset> TimerPresetRepository::findAll() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QStringLiteral("SELECT %1 FROM timer_presets "
                              "ORDER BY is_default DESC, id ASC")
                   .arg(kPresetColumns));

    QVector<TimerPreset> presets;
    while (query.next()) {
        presets.push_back(fromQuery(query));
    }
    return presets;
}

TimerPreset TimerPresetRepository::findById(int id) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("SELECT %1 FROM timer_presets WHERE id = :id")
                      .arg(kPresetColumns));
    query.bindValue(QStringLiteral(":id"), id);
    if (query.exec() && query.next()) {
        return fromQuery(query);
    }
    TimerPreset missing;
    missing.id = -1;
    missing.name.clear();
    return missing;
}

TimerPreset TimerPresetRepository::defaultPreset() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("SELECT %1 FROM timer_presets "
                                 "ORDER BY is_default DESC, id ASC LIMIT 1")
                      .arg(kPresetColumns));
    if (query.exec() && query.next()) {
        TimerPreset preset = fromQuery(query);
        preset.isDefault = true;
        return preset;
    }

    TimerPreset fallback;
    fallback.isDefault = true;
    return fallback;
}

bool TimerPresetRepository::save(TimerPreset &preset,
                                 QString *errorMessage) const
{
    if (!isValid(preset, errorMessage)) {
        return false;
    }

    if (preset.id > 0) {
        const TimerPreset existing = findById(preset.id);
        if (existing.id <= 0) {
            assignError(QStringLiteral("所选专注方案不存在。"), errorMessage);
            return false;
        }
        if (existing.isBuiltIn) {
            assignError(QStringLiteral(
                "内置预设方案不能修改，可以先复制一份再自定义。"),
                errorMessage);
            return false;
        }
    }

    auto database = DatabaseManager::instance().database();
    if (!database.transaction()) {
        assignError(database.lastError().text(), errorMessage);
        return false;
    }

    if (preset.isDefault) {
        QSqlQuery clearDefault(database);
        if (!clearDefault.exec(QStringLiteral(
                "UPDATE timer_presets SET is_default = 0"))) {
            assignError(clearDefault.lastError().text(), errorMessage);
            database.rollback();
            return false;
        }
    }

    QSqlQuery query(database);
    if (preset.id > 0) {
        query.prepare(QStringLiteral(R"(
            UPDATE timer_presets SET
                name = :name,
                focus_minutes = :focus_minutes,
                short_break_minutes = :short_break_minutes,
                long_break_minutes = :long_break_minutes,
                cycles_before_long_break = :cycles,
                breaks_enabled = :breaks_enabled,
                auto_start_break = :auto_break,
                auto_start_focus = :auto_focus,
                auto_start_next_focus = :auto_next_focus,
                is_default = :is_default
            WHERE id = :id
        )"));
        query.bindValue(QStringLiteral(":id"), preset.id);
    } else {
        QSqlQuery count(database);
        if (!count.exec(QStringLiteral("SELECT COUNT(*) FROM timer_presets"))
            || !count.next()) {
            assignError(count.lastError().text(), errorMessage);
            database.rollback();
            return false;
        }
        if (count.value(0).toInt() == 0) {
            preset.isDefault = true;
        }
        query.prepare(QStringLiteral(R"(
            INSERT INTO timer_presets(
                name, focus_minutes, short_break_minutes, long_break_minutes,
                cycles_before_long_break, breaks_enabled, auto_start_break,
                auto_start_focus, auto_start_next_focus, is_default
            ) VALUES(
                :name, :focus_minutes, :short_break_minutes, :long_break_minutes,
                :cycles, :breaks_enabled, :auto_break, :auto_focus,
                :auto_next_focus, :is_default
            )
        )"));
    }

    query.bindValue(QStringLiteral(":name"), preset.name.trimmed());
    query.bindValue(QStringLiteral(":focus_minutes"), preset.focusMinutes);
    query.bindValue(QStringLiteral(":short_break_minutes"),
                    preset.shortBreakMinutes);
    query.bindValue(QStringLiteral(":long_break_minutes"),
                    preset.longBreakMinutes);
    query.bindValue(QStringLiteral(":cycles"), preset.cyclesBeforeLongBreak);
    query.bindValue(QStringLiteral(":breaks_enabled"),
                    preset.breaksEnabled ? 1 : 0);
    query.bindValue(QStringLiteral(":auto_break"),
                    preset.breaksEnabled && preset.autoStartBreak ? 1 : 0);
    query.bindValue(QStringLiteral(":auto_focus"),
                    preset.breaksEnabled && preset.autoStartFocus ? 1 : 0);
    query.bindValue(QStringLiteral(":auto_next_focus"),
                    !preset.breaksEnabled && preset.autoStartNextFocus ? 1 : 0);
    query.bindValue(QStringLiteral(":is_default"),
                    preset.isDefault ? 1 : 0);

    if (!query.exec()) {
        QString error = query.lastError().text();
        if (error.contains(QStringLiteral("UNIQUE"),
                           Qt::CaseInsensitive)) {
            error = QStringLiteral("方案名称已经存在，请换一个名称。");
        }
        assignError(error, errorMessage);
        database.rollback();
        return false;
    }

    if (preset.id <= 0) {
        preset.id = query.lastInsertId().toInt();
        preset.isBuiltIn = false;
    }
    preset.name = preset.name.trimmed();
    if (preset.breaksEnabled) {
        preset.autoStartNextFocus = false;
    } else {
        preset.autoStartBreak = false;
        preset.autoStartFocus = false;
    }

    if (!database.commit()) {
        assignError(database.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

bool TimerPresetRepository::setDefault(int id,
                                       QString *errorMessage) const
{
    auto database = DatabaseManager::instance().database();
    if (!database.transaction()) {
        assignError(database.lastError().text(), errorMessage);
        return false;
    }

    QSqlQuery exists(database);
    exists.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM timer_presets WHERE id = :id"));
    exists.bindValue(QStringLiteral(":id"), id);
    if (!exists.exec() || !exists.next() || exists.value(0).toInt() != 1) {
        assignError(QStringLiteral("所选专注方案不存在。"), errorMessage);
        database.rollback();
        return false;
    }

    QSqlQuery clearDefault(database);
    if (!clearDefault.exec(QStringLiteral(
            "UPDATE timer_presets SET is_default = 0"))) {
        assignError(clearDefault.lastError().text(), errorMessage);
        database.rollback();
        return false;
    }

    QSqlQuery setDefault(database);
    setDefault.prepare(QStringLiteral(
        "UPDATE timer_presets SET is_default = 1 WHERE id = :id"));
    setDefault.bindValue(QStringLiteral(":id"), id);
    if (!setDefault.exec()) {
        assignError(setDefault.lastError().text(), errorMessage);
        database.rollback();
        return false;
    }

    if (!database.commit()) {
        assignError(database.lastError().text(), errorMessage);
        return false;
    }
    return true;
}

bool TimerPresetRepository::remove(int id,
                                   QString *errorMessage) const
{
    const TimerPreset preset = findById(id);
    if (preset.id <= 0) {
        assignError(QStringLiteral("所选专注方案不存在。"), errorMessage);
        return false;
    }
    if (preset.isBuiltIn) {
        assignError(QStringLiteral(
            "内置预设方案不能删除，可以复制后创建自己的方案。"),
            errorMessage);
        return false;
    }
    if (preset.isDefault) {
        assignError(QStringLiteral(
            "默认方案不能删除，请先把其他方案设为默认。"),
            errorMessage);
        return false;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral(
        "DELETE FROM timer_presets WHERE id = :id "
        "AND is_default = 0 AND is_builtin = 0"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        assignError(query.lastError().text(), errorMessage);
        return false;
    }
    if (query.numRowsAffected() != 1) {
        assignError(QStringLiteral("专注方案未能删除，请刷新后重试。"),
                    errorMessage);
        return false;
    }
    return true;
}

TimerPreset TimerPresetRepository::fromQuery(const QSqlQuery &query)
{
    TimerPreset preset;
    preset.id = query.value(QStringLiteral("id")).toInt();
    preset.name = query.value(QStringLiteral("name")).toString();
    preset.focusMinutes = query.value(QStringLiteral("focus_minutes")).toInt();
    preset.shortBreakMinutes =
        query.value(QStringLiteral("short_break_minutes")).toInt();
    preset.longBreakMinutes =
        query.value(QStringLiteral("long_break_minutes")).toInt();
    preset.cyclesBeforeLongBreak =
        query.value(QStringLiteral("cycles_before_long_break")).toInt();
    preset.breaksEnabled =
        query.value(QStringLiteral("breaks_enabled")).toInt() != 0;
    preset.autoStartBreak =
        query.value(QStringLiteral("auto_start_break")).toInt() != 0;
    preset.autoStartFocus =
        query.value(QStringLiteral("auto_start_focus")).toInt() != 0;
    preset.autoStartNextFocus =
        query.value(QStringLiteral("auto_start_next_focus")).toInt() != 0;
    preset.isDefault =
        query.value(QStringLiteral("is_default")).toInt() != 0;
    preset.isBuiltIn =
        query.value(QStringLiteral("is_builtin")).toInt() != 0;
    return preset;
}

bool TimerPresetRepository::isValid(const TimerPreset &preset,
                                    QString *errorMessage)
{
    if (preset.name.trimmed().isEmpty()) {
        assignError(QStringLiteral("请输入方案名称。"), errorMessage);
        return false;
    }
    if (preset.focusMinutes < 1 || preset.focusMinutes > 180
        || preset.shortBreakMinutes < 1 || preset.shortBreakMinutes > 60
        || preset.longBreakMinutes < 1 || preset.longBreakMinutes > 120
        || preset.cyclesBeforeLongBreak < 2
        || preset.cyclesBeforeLongBreak > 8) {
        assignError(QStringLiteral("专注方案的时间或周期超出允许范围。"),
                    errorMessage);
        return false;
    }
    return true;
}

void TimerPresetRepository::assignError(const QString &message,
                                        QString *destination)
{
    if (destination != nullptr) {
        *destination = message;
    }
}
