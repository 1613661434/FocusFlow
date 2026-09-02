#pragma once

#include <QString>

struct TimerPreset
{
    int id = -1;
    QString name = QStringLiteral("经典番茄钟");
    int focusMinutes = 25;
    int shortBreakMinutes = 5;
    int longBreakMinutes = 15;
    int cyclesBeforeLongBreak = 4;
    bool autoStartBreak = false;
    bool autoStartFocus = false;
    bool isDefault = false;
};
