#include "data/DatabaseManager.h"
#include "views/FocusPage.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>
#include <QUuid>

class FocusPageTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void primaryButtonTracksTimerState();
    void cleanupTestCase();

private:
    static QPushButton *buttonForRole(FocusPage &page, const QString &role);
    QString dataDirectory_;
};

void FocusPageTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlowTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("FocusPage-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QVERIFY2(DatabaseManager::instance().initialize(),
             qPrintable(DatabaseManager::instance().lastError()));
    dataDirectory_ = QFileInfo(
        DatabaseManager::instance().databasePath()).absolutePath();
}

QPushButton *FocusPageTests::buttonForRole(FocusPage &page,
                                           const QString &role)
{
    const auto buttons = page.findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->property("timerControl").toString() == role) {
            return button;
        }
    }
    return nullptr;
}

void FocusPageTests::primaryButtonTracksTimerState()
{
    FocusPage page;
    auto *primary = buttonForRole(page, QStringLiteral("primary"));
    auto *stop = buttonForRole(page, QStringLiteral("stop"));
    QVERIFY(primary);
    QVERIFY(stop);
    QCOMPARE(primary->text(), QStringLiteral("开始"));
    QVERIFY(primary->isEnabled());
    QVERIFY(!stop->isEnabled());
    QVERIFY(primary->minimumHeight() >= 44);
    QCOMPARE(primary->minimumSize(), stop->minimumSize());

    primary->click();
    QCOMPARE(primary->text(), QStringLiteral("暂停"));
    QVERIFY(stop->isEnabled());

    primary->click();
    QCOMPARE(primary->text(), QStringLiteral("继续"));
    QVERIFY(stop->isEnabled());

    primary->click();
    QCOMPARE(primary->text(), QStringLiteral("暂停"));

    QTimer::singleShot(0, [] {
        if (auto *dialog = qobject_cast<QMessageBox *>(
                QApplication::activeModalWidget())) {
            dialog->button(QMessageBox::Yes)->click();
        }
    });
    stop->click();
    QCOMPARE(primary->text(), QStringLiteral("开始"));
    QVERIFY(primary->isEnabled());
    QVERIFY(!stop->isEnabled());
}

void FocusPageTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(FocusPageTests)

#include "tst_FocusPage.moc"
