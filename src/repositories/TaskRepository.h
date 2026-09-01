#pragma once

#include "models/Task.h"

#include <QString>
#include <QVector>

class TaskRepository final
{
public:
    enum class Filter {
        All,
        Recommended,
        Today,
        ThisWeek,
        Overdue,
        Completed,
    };

    QVector<Task> findAll(Filter filter = Filter::All,
                          const QString &searchText = {}) const;
    Task findById(int id) const;
    QVector<LookupItem> projects() const;
    QVector<LookupItem> categories() const;

    bool save(Task &task, QString *errorMessage = nullptr) const;
    bool setCompleted(int id, bool completed, QString *errorMessage = nullptr) const;
    bool deleteTask(int id, QString *errorMessage = nullptr) const;

private:
    static Task fromQuery(const class QSqlQuery &query);
    static void assignError(const QString &message, QString *destination);
};
