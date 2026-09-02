#pragma once

#include <QString>
#include <QVector>

struct DashboardMetrics
{
    int pendingTasks = 0;
    int dueToday = 0;
    int overdueTasks = 0;
    int completedToday = 0;
    int focusSecondsToday = 0;
    int focusSecondsLastSevenDays = 0;
};

struct DailyProductivity
{
    QString label;
    int focusSeconds = 0;
    int completedTasks = 0;
};

struct CategoryFocus
{
    QString name;
    QString color;
    int focusSeconds = 0;
};

struct ProjectFocus
{
    QString name;
    QString color;
    int focusSeconds = 0;
};

struct RecentFocusSession
{
    QString startedAt;
    QString taskName;
    QString projectName;
    QString projectColor;
    QString categoryName;
    QString categoryColor;
    int focusSeconds = 0;
    bool completed = false;
};

class AnalyticsRepository final
{
public:
    DashboardMetrics dashboardMetrics() const;
    QVector<DailyProductivity> lastSevenDays() const;
    QVector<CategoryFocus> focusByCategory() const;
    QVector<ProjectFocus> focusByProject() const;
    QVector<RecentFocusSession> recentFocusSessions(int limit = 10) const;

private:
    int scalar(const QString &sql) const;
};
