#pragma once

#include <QString>

struct TimerSettings
{
    int focusMinutes = 25;
    int shortBreakMinutes = 5;
    int longBreakMinutes = 15;
    int cyclesBeforeLongBreak = 4;
    bool autoStartBreak = false;
    bool autoStartFocus = false;

    bool soundEnabled = true;
    QString focusSoundPath;
    QString breakSoundPath;
    int volumePercent = 70;
    bool playFullSound = false;
    int maxSoundSeconds = 5;
    int soundRepeatCount = 1;

    bool suppressCloseToTrayReminder = false;
};
