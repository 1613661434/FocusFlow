#include "views/StatisticsPage.h"

#include "repositories/AnalyticsRepository.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLegend>
#include <QPieSeries>
#include <QPieSlice>
#include <QValueAxis>
#include <QVBoxLayout>

StatisticsPage::StatisticsPage(QWidget *parent)
    : QWidget(parent)
{
    buildInterface();
    refresh();
}

void StatisticsPage::buildInterface()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    auto createSummary = [this](const QString &title, QLabel **value) {
        auto *card = new QFrame(this);
        card->setObjectName(QStringLiteral("card"));
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 14, 18, 14);
        auto *label = new QLabel(title, card);
        label->setObjectName(QStringLiteral("mutedLabel"));
        *value = new QLabel(QStringLiteral("0"), card);
        (*value)->setObjectName(QStringLiteral("metricValue"));
        layout->addWidget(label);
        layout->addWidget(*value);
        return card;
    };

    auto *summary = new QHBoxLayout;
    summary->addWidget(createSummary(QStringLiteral("今日专注"), &todayFocusValue_));
    summary->addWidget(createSummary(QStringLiteral("近7天专注"), &weekFocusValue_));
    summary->addWidget(createSummary(QStringLiteral("今日完成任务"), &completedValue_));

    dailyChartView_ = new QChartView(this);
    dailyChartView_->setRenderHint(QPainter::Antialiasing);
    dailyChartView_->setMinimumHeight(320);
    categoryChartView_ = new QChartView(this);
    categoryChartView_->setRenderHint(QPainter::Antialiasing);
    categoryChartView_->setMinimumHeight(320);

    auto *charts = new QHBoxLayout;
    charts->setSpacing(16);
    charts->addWidget(dailyChartView_, 3);
    charts->addWidget(categoryChartView_, 2);

    root->addLayout(summary);
    root->addLayout(charts, 1);
}

void StatisticsPage::refresh()
{
    const DashboardMetrics metrics = AnalyticsRepository().dashboardMetrics();
    todayFocusValue_->setText(formatDuration(metrics.focusSecondsToday));
    weekFocusValue_->setText(formatDuration(metrics.focusSecondsLastSevenDays));
    completedValue_->setText(QString::number(metrics.completedToday));
    updateDailyChart();
    updateCategoryChart();
}

void StatisticsPage::updateDailyChart()
{
    const auto daily = AnalyticsRepository().lastSevenDays();
    auto *focusSet = new QBarSet(QStringLiteral("专注分钟"));
    QStringList labels;
    int maximum = 30;
    for (const auto &day : daily) {
        *focusSet << day.focusMinutes;
        labels << day.label;
        maximum = qMax(maximum, day.focusMinutes + 10);
    }

    auto *series = new QBarSeries;
    series->append(focusSet);
    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("近7天专注时长"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(labels);
    auto *axisY = new QValueAxis;
    axisY->setRange(0, maximum);
    axisY->setTitleText(QStringLiteral("分钟"));
    axisY->setLabelFormat(QStringLiteral("%d"));
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    dailyChartView_->setChart(chart);
}

void StatisticsPage::updateCategoryChart()
{
    const auto categories = AnalyticsRepository().focusByCategory();
    auto *series = new QPieSeries;
    for (const auto &category : categories) {
        series->append(category.name, category.focusMinutes);
    }
    if (categories.isEmpty()) {
        series->append(QStringLiteral("暂无数据"), 1);
    }
    for (auto *slice : series->slices()) {
        slice->setLabelVisible(true);
        slice->setLabel(QStringLiteral("%1 %2%").arg(slice->label()).arg(slice->percentage() * 100.0, 0, 'f', 0));
    }

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("近30天分类专注占比"));
    chart->legend()->setVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    categoryChartView_->setChart(chart);
}

QString StatisticsPage::formatDuration(int seconds)
{
    const int minutes = qMax(0, seconds) / 60;
    if (seconds > 0 && minutes == 0) {
        return QStringLiteral("不足 1 分钟");
    }
    return minutes < 60
        ? QStringLiteral("%1 分钟").arg(minutes)
        : QStringLiteral("%1 小时 %2 分钟").arg(minutes / 60).arg(minutes % 60);
}
