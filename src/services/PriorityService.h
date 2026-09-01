#pragma once

#include "models/Task.h"

#include <QDateTime>

class PriorityService final
{
public:
    static int score(const Task &task,
                     const QDateTime &now = QDateTime::currentDateTime());
};
