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
    auto *focusSet = new QBarSet(QStringLiteral("专注秒数"));
    QStringList labels;
    int maximum = 60;
    for (const auto &day : daily) {
        *focusSet << day.focusSeconds;
        labels << day.label;
        maximum = qMax(maximum, day.focusSeconds + 30);
    }

    auto *series = new QBarSeries;
    series->append(focusSet);
    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("近7天专注时长（秒）"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(labels);
    auto *axisY = new QValueAxis;
    axisY->setRange(0, maximum);
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
        series->append(category.name, category.focusSeconds);
    }
    if (categories.isEmpty()) {
        series->append(QStringLiteral("暂无数据"), 1);
    }
    const auto slices = series->slices();
    for (qsizetype index = 0; index < slices.size(); ++index) {
        auto *slice = slices.at(index);
        slice->setLabelVisible(true);
        if (categories.isEmpty()) {
            slice->setLabel(QStringLiteral("暂无数据"));
        } else {
            slice->setLabel(QStringLiteral("%1 %2% · %3")
                                .arg(categories.at(index).name)
                                .arg(slice->percentage() * 100.0, 0, 'f', 0)
                                .arg(formatDuration(categories.at(index).focusSeconds)));
        }
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
    const int safeSeconds = qMax(0, seconds);
    const int hours = safeSeconds / 3600;
    const int minutes = (safeSeconds % 3600) / 60;
    const int remainingSeconds = safeSeconds % 60;
    if (hours > 0) {
        return QStringLiteral("%1 小时 %2 分 %3 秒")
            .arg(hours)
            .arg(minutes)
            .arg(remainingSeconds);
    }
    if (minutes > 0) {
        return QStringLiteral("%1 分 %2 秒")
            .arg(minutes)
            .arg(remainingSeconds);
    }
    return QStringLiteral("%1 秒").arg(remainingSeconds);
}
