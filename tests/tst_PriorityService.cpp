#include "services/PriorityService.h"

#include <QTest>

class PriorityServiceTests final : public QObject
{
    Q_OBJECT

private slots:
    void importanceRaisesScore();
    void overdueTaskGetsUrgencyBonus();
    void completedTaskHasZeroScore();
    void shortTaskGetsQuickWinBonus();
};

void PriorityServiceTests::importanceRaisesScore()
{
    const QDateTime now(QDate(2026, 9, 1), QTime(9, 0));
    Task normal;
    normal.importance = 2;
    Task important = normal;
    important.importance = 5;
    QVERIFY(PriorityService::score(important, now)
            > PriorityService::score(normal, now));
}

void PriorityServiceTests::overdueTaskGetsUrgencyBonus()
{
    const QDateTime now(QDate(2026, 9, 1), QTime(9, 0));
    Task overdue;
    overdue.importance = 3;
    overdue.dueAt = now.addDays(-2);
    Task later = overdue;
    later.dueAt = now.addDays(10);
    QVERIFY(PriorityService::score(overdue, now)
            > PriorityService::score(later, now));
}

void PriorityServiceTests::completedTaskHasZeroScore()
{
    Task task;
    task.importance = 5;
    task.dueAt = QDateTime::currentDateTime().addDays(-5);
    task.status = QStringLiteral("completed");
    QCOMPARE(PriorityService::score(task), 0);
}

void PriorityServiceTests::shortTaskGetsQuickWinBonus()
{
    const QDateTime now(QDate(2026, 9, 1), QTime(9, 0));
    Task shortTask;
    shortTask.estimatedMinutes = 20;
    Task longTask = shortTask;
    longTask.estimatedMinutes = 90;
    QCOMPARE(PriorityService::score(shortTask, now)
                 - PriorityService::score(longTask, now),
             5);
}

QTEST_MAIN(PriorityServiceTests)

#include "tst_PriorityService.moc"
