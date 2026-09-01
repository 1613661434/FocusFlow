#include "data/DatabaseManager.h"
#include "views/DashboardPage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTest>
#include <QUuid>

namespace {
QLabel *findLabelByText(const QWidget &parent, const QString &text)
{
    const auto labels = parent.findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label->text() == text) {
            return label;
        }
    }
    return nullptr;
}
}

class DashboardPageTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void emptyStateKeepsContentTopAligned();
    void cleanupTestCase();

private:
    QString dataDirectory_;
};

void DashboardPageTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlowTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("Dashboard-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));

    QVERIFY2(DatabaseManager::instance().initialize(),
             qPrintable(DatabaseManager::instance().lastError()));
    dataDirectory_ =
        QFileInfo(DatabaseManager::instance().databasePath()).absolutePath();
}

void DashboardPageTests::emptyStateKeepsContentTopAligned()
{
    DashboardPage page;
    page.resize(1200, 700);
    page.show();
    QCoreApplication::processEvents();

    auto *title = findLabelByText(page, QStringLiteral("优先建议"));
    auto *description = findLabelByText(
        page, QStringLiteral("综合重要程度、截止时间、逾期情况和预计耗时排序。"));
    auto *emptyState =
        page.findChild<QLabel *>(QStringLiteral("emptyStateLabel"));
    auto *content =
        page.findChild<QStackedWidget *>(QStringLiteral("recommendationContent"));
    auto *list =
        page.findChild<QListWidget *>(QStringLiteral("recommendationList"));

    QVERIFY(title != nullptr);
    QVERIFY(description != nullptr);
    QVERIFY(emptyState != nullptr);
    QVERIFY(content != nullptr);
    QVERIFY(list != nullptr);
    QCOMPARE(content->currentIndex(), 1);
    QVERIFY(emptyState->isVisible());
    QVERIFY(!list->isVisible());
    QVERIFY(emptyState->testAttribute(Qt::WA_TransparentForMouseEvents));

    QWidget *card = title->parentWidget();
    QVERIFY(card != nullptr);
    const int titleTop = title->mapTo(card, QPoint(0, 0)).y();
    const int titleBottom = titleTop + title->height();
    const int descriptionTop = description->mapTo(card, QPoint(0, 0)).y();
    const int descriptionBottom = descriptionTop + description->height();
    const int emptyStateTop = emptyState->mapTo(card, QPoint(0, 0)).y();

    QVERIFY(titleTop <= 30);
    QVERIFY(descriptionTop >= titleBottom);
    QVERIFY(descriptionTop - titleBottom <= 16);
    QVERIFY(emptyStateTop >= descriptionBottom);
    QVERIFY(emptyStateTop - descriptionBottom <= 20);
    QVERIFY(emptyStateTop < card->height() / 4);
}

void DashboardPageTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(DashboardPageTests)

#include "tst_DashboardPage.moc"
