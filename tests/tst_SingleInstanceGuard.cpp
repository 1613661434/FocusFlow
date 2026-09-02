#include "app/SingleInstanceGuard.h"

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include <future>

class SingleInstanceGuardTests final : public QObject
{
    Q_OBJECT

private slots:
    void secondInstanceRequestsActivation();
    void lockIsReleasedWithPrimaryInstance();
};

void SingleInstanceGuardTests::secondInstanceRequestsActivation()
{
    const QString instanceKey = QUuid::createUuid().toString();
    SingleInstanceGuard primary(instanceKey);
    QCOMPARE(primary.start(), SingleInstanceGuard::StartResult::Primary);

    QSignalSpy activationSpy(&primary,
                             &SingleInstanceGuard::activationRequested);
    auto secondaryResult = std::async(std::launch::async, [instanceKey] {
        SingleInstanceGuard secondary(instanceKey);
        return secondary.start();
    });
    QTRY_COMPARE_WITH_TIMEOUT(activationSpy.count(), 1, 2000);
    QCOMPARE(secondaryResult.get(),
             SingleInstanceGuard::StartResult::Secondary);
}

void SingleInstanceGuardTests::lockIsReleasedWithPrimaryInstance()
{
    const QString instanceKey = QUuid::createUuid().toString();
    {
        SingleInstanceGuard primary(instanceKey);
        QCOMPARE(primary.start(), SingleInstanceGuard::StartResult::Primary);
    }

    SingleInstanceGuard replacement(instanceKey);
    QCOMPARE(replacement.start(), SingleInstanceGuard::StartResult::Primary);
}

QTEST_GUILESS_MAIN(SingleInstanceGuardTests)

#include "tst_SingleInstanceGuard.moc"
