#include "widgets/ChineseCalendarWidget.h"

#include <QHBoxLayout>
#include <QLocale>
#include <QToolButton>
#include <QWidget>

ChineseCalendarWidget::ChineseCalendarWidget(QWidget *parent)
    : QCalendarWidget(parent)
{
    setLocale(QLocale(QLocale::Chinese, QLocale::China));
    setFirstDayOfWeek(Qt::Monday);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    connect(this,
            &QCalendarWidget::currentPageChanged,
            this,
            &ChineseCalendarWidget::updateNavigationHeader);
    updateNavigationHeader(yearShown(), monthShown());
}

void ChineseCalendarWidget::updateNavigationHeader(int year, int month)
{
    auto *navigationBar =
        findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));
    auto *yearButton =
        findChild<QToolButton *>(QStringLiteral("qt_calendar_yearbutton"));
    auto *monthButton =
        findChild<QToolButton *>(QStringLiteral("qt_calendar_monthbutton"));
    auto *previousButton =
        findChild<QToolButton *>(QStringLiteral("qt_calendar_prevmonth"));
    auto *nextButton =
        findChild<QToolButton *>(QStringLiteral("qt_calendar_nextmonth"));

    if (yearButton != nullptr) {
        yearButton->setText(QStringLiteral("%1年").arg(year));
        yearButton->setToolTip(QStringLiteral("选择年份"));
        yearButton->setAccessibleName(QStringLiteral("选择年份"));
    }
    if (monthButton != nullptr) {
        monthButton->setText(QStringLiteral("%1月").arg(month));
        monthButton->setToolTip(QStringLiteral("选择月份"));
        monthButton->setAccessibleName(QStringLiteral("选择月份"));
    }
    if (previousButton != nullptr) {
        previousButton->setToolTip(QStringLiteral("上一月"));
        previousButton->setAccessibleName(QStringLiteral("上一月"));
    }
    if (nextButton != nullptr) {
        nextButton->setToolTip(QStringLiteral("下一月"));
        nextButton->setAccessibleName(QStringLiteral("下一月"));
    }

    if (navigationBar == nullptr || yearButton == nullptr || monthButton == nullptr) {
        return;
    }
    auto *layout = qobject_cast<QHBoxLayout *>(navigationBar->layout());
    if (layout == nullptr) {
        return;
    }

    const int yearIndex = layout->indexOf(yearButton);
    const int monthIndex = layout->indexOf(monthButton);
    if (yearIndex < 0 || monthIndex < 0 || yearIndex < monthIndex) {
        return;
    }

    const int insertionIndex = qMin(yearIndex, monthIndex);
    layout->removeWidget(yearButton);
    layout->removeWidget(monthButton);
    layout->insertWidget(insertionIndex, yearButton);
    layout->insertWidget(insertionIndex + 1, monthButton);
}
