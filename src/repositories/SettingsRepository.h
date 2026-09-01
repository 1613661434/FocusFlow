#pragma once

#include "models/TimerSettings.h"

#include <QString>

class SettingsRepository final
{
public:
    TimerSettings loadTimerSettings() const;
    bool saveTimerSettings(const TimerSettings &settings,
                           QString *errorMessage = nullptr) const;

private:
    QString value(const QString &key, const QString &fallback) const;
};
