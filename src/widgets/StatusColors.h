#pragma once

#include <QColor>
#include <QString>

namespace StatusColors {

inline QColor success()
{
    return QColor(QStringLiteral("#067647"));
}

inline QColor pending()
{
    return QColor(QStringLiteral("#175CD3"));
}

inline QColor interrupted()
{
    return QColor(QStringLiteral("#B54708"));
}

inline QColor muted()
{
    return QColor(QStringLiteral("#667085"));
}

inline QColor inProgress()
{
    return QColor(QStringLiteral("#6941C6"));
}

inline QColor danger()
{
    return QColor(QStringLiteral("#B42318"));
}

inline QColor taskStatus(const QString &status)
{
    if (status == QStringLiteral("completed")) {
        return success();
    }
    if (status == QStringLiteral("in_progress")) {
        return inProgress();
    }
    if (status == QStringLiteral("cancelled")) {
        return danger();
    }
    return pending();
}

inline QColor projectStatus(bool archived)
{
    return archived ? muted() : success();
}

inline QColor focusResult(bool completed)
{
    return completed ? success() : interrupted();
}

}
