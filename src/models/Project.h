#pragma once

#include <QString>

struct Project
{
    int id = -1;
    QString name;
    QString description;
    QString color = QStringLiteral("#4F6EF7");
    bool archived = false;
};
