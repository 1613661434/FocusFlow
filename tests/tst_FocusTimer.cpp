#include "services/FocusTimer.h"

#include <QSignalSpy>
#include <QTest>

class FocusTimerTests final : public QObject
{
    Q_OBJECT

private slots:
    void startsInIdleState();
    void completesOneSecondSession();
    void pauseFreezesRemainingTime();
};

void FocusTimerTests::startsInIdleState()
{
    FocusTimer timer;
    QCOMPARE(timer.state(), FocusTimer::State::Idle);
    QCOMPARE(timer.remainingSeconds(), 0);
    QCOMPARE(timer.actualSeconds(), 0);
}

void FocusTimerTests::completesOneSecondSession()
{
    FocusTimer timer;
    QSignalSpy endedSpy(&timer, &FocusTimer::sessionEnded);

    timer.start(FocusTimer::Phase::Focus, 1, 42);
    QCOMPARE(timer.state(), FocusTimer::State::Running);
    QTRY_COMPARE_WITH_TIMEOUT(endedSpy.count(), 1, 2000);
    QCOMPARE(timer.state(), FocusTimer::State::Idle);

    const QList<QVariant> arguments = endedSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<FocusTimer::Phase>(), FocusTimer::Phase::Focus);
    QCOMPARE(arguments.at(1).toBool(), true);
    QCOMPARE(arguments.at(2).toInt(), 42);
    QCOMPARE(arguments.at(5).toInt(), 1);
    QCOMPARE(arguments.at(6).toInt(), 1);
}

void FocusTimerTests::pauseFreezesRemainingTime()
{
    FocusTimer timer;
    QSignalSpy endedSpy(&timer, &FocusTimer::sessionEnded);

    timer.start(FocusTimer::Phase::ShortBreak, 2);
    QTest::qWait(300);
    timer.pause();
    QCOMPARE(timer.state(), FocusTimer::State::Paused);
    const int pausedRemaining = timer.remainingSeconds();

    QTest::qWait(600);
    QCOMPARE(timer.remainingSeconds(), pausedRemaining);
    QCOMPARE(endedSpy.count(), 0);

    timer.resume();
    QCOMPARE(timer.state(), FocusTimer::State::Running);
    QTRY_COMPARE_WITH_TIMEOUT(endedSpy.count(), 1, 3000);
}

QTEST_MAIN(FocusTimerTests)

#include "tst_FocusTimer.moc"
