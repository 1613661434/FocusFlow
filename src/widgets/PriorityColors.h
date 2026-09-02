#pragma once

#include <QColor>
#include <QtGlobal>

namespace PriorityColors {

inline QColor importance(int level)
{
    switch (qBound(1, level, 5)) {
    case 1:
        return QColor(QStringLiteral("#667085"));
    case 2:
        return QColor(QStringLiteral("#175CD3"));
    case 3:
        return QColor(QStringLiteral("#6941C6"));
    case 4:
        return QColor(QStringLiteral("#B54708"));
    case 5:
    default:
        return QColor(QStringLiteral("#B42318"));
    }
}

inline QColor recommendation(int score)
{
    if (score < 40) {
        return QColor(QStringLiteral("#667085"));
    }
    if (score < 70) {
        return QColor(QStringLiteral("#175CD3"));
    }
    if (score < 100) {
        return QColor(QStringLiteral("#6941C6"));
    }
    if (score < 130) {
        return QColor(QStringLiteral("#B54708"));
    }
    return QColor(QStringLiteral("#B42318"));
}

}
