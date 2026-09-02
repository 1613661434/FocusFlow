#pragma once

#include "models/TimerPreset.h"

#include <QString>
#include <QVector>

class TimerPresetRepository final
{
public:
    QVector<TimerPreset> findAll() const;
    TimerPreset findById(int id) const;
    TimerPreset defaultPreset() const;

    bool save(TimerPreset &preset, QString *errorMessage = nullptr) const;
    bool setDefault(int id, QString *errorMessage = nullptr) const;
    bool remove(int id, QString *errorMessage = nullptr) const;

private:
    static TimerPreset fromQuery(const class QSqlQuery &query);
    static bool isValid(const TimerPreset &preset, QString *errorMessage);
    static void assignError(const QString &message, QString *destination);
};
