#include "repositories/SettingsRepository.h"

#include "data/DatabaseManager.h"

#include <QHash>
#include <QSqlError>
#include <QSqlQuery>

TimerSettings SettingsRepository::loadTimerSettings() const
{
    TimerSettings settings;
    settings.focusMinutes = value(QStringLiteral("timer.focus_minutes"),
                                  QString::number(settings.focusMinutes)).toInt();
    settings.shortBreakMinutes = value(QStringLiteral("timer.short_break_minutes"),
                                       QString::number(settings.shortBreakMinutes)).toInt();
    settings.longBreakMinutes = value(QStringLiteral("timer.long_break_minutes"),
                                      QString::number(settings.longBreakMinutes)).toInt();
    settings.cyclesBeforeLongBreak = value(
        QStringLiteral("timer.cycles_before_long_break"),
        QString::number(settings.cyclesBeforeLongBreak)).toInt();
    settings.autoStartBreak = value(QStringLiteral("timer.auto_start_break"),
                                    QStringLiteral("0")) == QStringLiteral("1");
    settings.autoStartFocus = value(QStringLiteral("timer.auto_start_focus"),
                                    QStringLiteral("0")) == QStringLiteral("1");
    settings.soundEnabled = value(QStringLiteral("sound.enabled"),
                                  QStringLiteral("1")) == QStringLiteral("1");
    settings.focusSoundPath = value(QStringLiteral("sound.focus_path"), {});
    settings.breakSoundPath = value(QStringLiteral("sound.break_path"), {});
    settings.volumePercent = value(QStringLiteral("sound.volume_percent"),
                                   QString::number(settings.volumePercent)).toInt();
    settings.maxSoundSeconds = value(QStringLiteral("sound.max_seconds"),
                                     QString::number(settings.maxSoundSeconds)).toInt();
    settings.soundRepeatCount = value(QStringLiteral("sound.repeat_count"),
                                      QString::number(settings.soundRepeatCount)).toInt();
    settings.suppressCloseToTrayReminder = value(
        QStringLiteral("window.suppress_close_to_tray_reminder"),
        QStringLiteral("0")) == QStringLiteral("1");
    return settings;
}

bool SettingsRepository::saveTimerSettings(const TimerSettings &settings,
                                           QString *errorMessage) const
{
    const QHash<QString, QString> values{
        {QStringLiteral("timer.focus_minutes"), QString::number(settings.focusMinutes)},
        {QStringLiteral("timer.short_break_minutes"), QString::number(settings.shortBreakMinutes)},
        {QStringLiteral("timer.long_break_minutes"), QString::number(settings.longBreakMinutes)},
        {QStringLiteral("timer.cycles_before_long_break"),
         QString::number(settings.cyclesBeforeLongBreak)},
        {QStringLiteral("timer.auto_start_break"), settings.autoStartBreak ? QStringLiteral("1") : QStringLiteral("0")},
        {QStringLiteral("timer.auto_start_focus"), settings.autoStartFocus ? QStringLiteral("1") : QStringLiteral("0")},
        {QStringLiteral("sound.enabled"), settings.soundEnabled ? QStringLiteral("1") : QStringLiteral("0")},
        {QStringLiteral("sound.focus_path"),
         settings.focusSoundPath.isNull() ? QStringLiteral("") : settings.focusSoundPath},
        {QStringLiteral("sound.break_path"),
         settings.breakSoundPath.isNull() ? QStringLiteral("") : settings.breakSoundPath},
        {QStringLiteral("sound.volume_percent"), QString::number(settings.volumePercent)},
        {QStringLiteral("sound.max_seconds"), QString::number(settings.maxSoundSeconds)},
        {QStringLiteral("sound.repeat_count"), QString::number(settings.soundRepeatCount)},
        {QStringLiteral("window.suppress_close_to_tray_reminder"),
         settings.suppressCloseToTrayReminder ? QStringLiteral("1")
                                                : QStringLiteral("0")},
    };

    auto database = DatabaseManager::instance().database();
    if (!database.transaction()) {
        if (errorMessage != nullptr) {
            *errorMessage = database.lastError().text();
        }
        return false;
    }

    QSqlQuery query(database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO settings(key, value) VALUES(:key, :value)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value
    )"));
    for (auto iterator = values.cbegin(); iterator != values.cend(); ++iterator) {
        query.bindValue(QStringLiteral(":key"), iterator.key());
        query.bindValue(QStringLiteral(":value"), iterator.value());
        if (!query.exec()) {
            if (errorMessage != nullptr) {
                *errorMessage = query.lastError().text();
            }
            database.rollback();
            return false;
        }
    }

    if (!database.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = database.lastError().text();
        }
        return false;
    }
    return true;
}

QString SettingsRepository::value(const QString &key, const QString &fallback) const
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("SELECT value FROM settings WHERE key = :key"));
    query.bindValue(QStringLiteral(":key"), key);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return fallback;
}
