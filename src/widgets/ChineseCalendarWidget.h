#pragma once

#include <QCalendarWidget>

class ChineseCalendarWidget final : public QCalendarWidget
{
public:
    explicit ChineseCalendarWidget(QWidget *parent = nullptr);

private:
    void updateNavigationHeader(int year, int month);
};
