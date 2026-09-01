#include "widgets/ChineseCalendarWidget.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QTest>
#include <QToolButton>
#include <QWidget>

class ChineseCalendarWidgetTests final : public QObject
{
    Q_OBJECT

private slots:
    void showsYearBeforeMonth();
    void updatesLocalizedHeaderAfterPageChange();
};

void ChineseCalendarWidgetTests::showsYearBeforeMonth()
{
    ChineseCalendarWidget calendar;
    calendar.setCurrentPage(2027, 9);
    QCoreApplication::processEvents();

    auto *yearButton =
        calendar.findChild<QToolButton *>(QStringLiteral("qt_calendar_yearbutton"));
    auto *monthButton =
        calendar.findChild<QToolButton *>(QStringLiteral("qt_calendar_monthbutton"));
    auto *navigationBar =
        calendar.findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));

    QVERIFY(yearButton != nullptr);
    QVERIFY(monthButton != nullptr);
    QVERIFY(navigationBar != nullptr);
    QCOMPARE(yearButton->text(), QStringLiteral("2027年"));
    QCOMPARE(monthButton->text(), QStringLiteral("9月"));

    auto *layout = qobject_cast<QHBoxLayout *>(navigationBar->layout());
    QVERIFY(layout != nullptr);
    QVERIFY(layout->indexOf(yearButton) < layout->indexOf(monthButton));
}

void ChineseCalendarWidgetTests::updatesLocalizedHeaderAfterPageChange()
{
    ChineseCalendarWidget calendar;
    calendar.setCurrentPage(2030, 12);
    QCoreApplication::processEvents();

    auto *yearButton =
        calendar.findChild<QToolButton *>(QStringLiteral("qt_calendar_yearbutton"));
    auto *monthButton =
        calendar.findChild<QToolButton *>(QStringLiteral("qt_calendar_monthbutton"));
    QVERIFY(yearButton != nullptr);
    QVERIFY(monthButton != nullptr);
    QCOMPARE(yearButton->text(), QStringLiteral("2030年"));
    QCOMPARE(monthButton->text(), QStringLiteral("12月"));
}

QTEST_MAIN(ChineseCalendarWidgetTests)

#include "tst_ChineseCalendarWidget.moc"
