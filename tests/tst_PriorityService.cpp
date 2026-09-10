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
    void inProgressStatusDoesNotChangeScore();
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

void PriorityServiceTests::inProgressStatusDoesNotChangeScore()
{
    const QDateTime now(QDate(2026, 9, 1), QTime(9, 0));
    Task pending;
    pending.importance = 4;
    pending.dueAt = now.addDays(2);
    pending.estimatedMinutes = 20;
    Task inProgress = pending;
    inProgress.status = QStringLiteral("in_progress");

    QCOMPARE(PriorityService::score(inProgress, now),
             PriorityService::score(pending, now));
}

QTEST_MAIN(PriorityServiceTests)

#include "tst_PriorityService.moc"
