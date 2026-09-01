#pragma once

#include <QWidget>

class QLabel;
class QChartView;
class QTableWidget;

class StatisticsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    void buildInterface();
    void updateDailyChart();
    void updateCategoryChart();
    void updateProjectChart();
    void updateRecentSessions();
    static QString formatDuration(int seconds);

    QLabel *weekFocusValue_ = nullptr;
    QLabel *todayFocusValue_ = nullptr;
    QLabel *completedValue_ = nullptr;
    QChartView *dailyChartView_ = nullptr;
    QChartView *categoryChartView_ = nullptr;
    QChartView *projectChartView_ = nullptr;
    QTableWidget *recentSessions_ = nullptr;
};
