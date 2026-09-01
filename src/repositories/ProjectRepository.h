#pragma once

#include "models/Project.h"
#include "models/Task.h"

#include <QString>
#include <QVector>

class ProjectRepository final
{
public:
    QVector<Project> projects(bool includeArchived = true) const;
    QVector<LookupItem> categories() const;

    bool saveProject(Project &project, QString *errorMessage = nullptr) const;
    bool setProjectArchived(int id, bool archived, QString *errorMessage = nullptr) const;
    bool saveCategory(LookupItem &category, QString *errorMessage = nullptr) const;
    bool deleteCategory(int id, QString *errorMessage = nullptr) const;

private:
    static void assignError(const QString &message, QString *destination);
};
