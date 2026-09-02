#pragma once

#include <QDateTime>
#include <QString>

struct Task
{
    int id = -1;
    QString title;
    QString description;
    int projectId = -1;
    QString projectName;
    QString projectColor;
    int categoryId = -1;
    QString categoryName;
    QString categoryColor;
    int importance = 3;
    QDateTime dueAt;
    int estimatedMinutes = 25;
    QString status = QStringLiteral("pending");
    QDateTime createdAt;
    QDateTime completedAt;
};

struct LookupItem
{
    int id = -1;
    QString name;
    QString color;
};
