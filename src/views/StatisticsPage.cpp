#include "views/StatisticsPage.h"

#include "repositories/AnalyticsRepository.h"
#include "widgets/ClearSelectionOnBlankClick.h"
#include "widgets/SortKeyTableWidgetItem.h"
#include "widgets/StatusColors.h"

#include <QAbstractItemView>
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLegend>
#include <QPieSeries>
#include <QPieSlice>
#include <QHeaderView>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
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

    auto *tabs = new QTabWidget(this);
    tabs->setObjectName(QStringLiteral("statisticsTabs"));
    tabs->tabBar()->setObjectName(QStringLiteral("statisticsModeSwitch"));
    tabs->setTabPosition(QTabWidget::North);
    tabs->setMovable(false);
    tabs->setUsesScrollButtons(false);
    tabs->setStyleSheet(QStringLiteral(R"(
        QTabWidget#statisticsTabs::pane {
            background: transparent;
            border: none;
            top: 0px;
        }
        QTabBar#statisticsModeSwitch {
            background: #eef2f7;
            border: 1px solid #dce2ec;
            border-radius: 9px;
        }
        QTabBar#statisticsModeSwitch::tab {
            min-width: 128px;
            min-height: 20px;
            color: #475467;
            background: transparent;
            border: none;
            border-radius: 7px;
            padding: 8px 18px;
            margin: 2px;
            font-weight: 600;
        }
        QTabBar#statisticsModeSwitch::tab:hover:!selected {
            color: #182230;
            background: #e4e9f2;
        }
        QTabBar#statisticsModeSwitch::tab:selected {
            color: #ffffff;
            background: #4f6ef7;
        }
    )"));

    auto *chartsPage = new QWidget(tabs);
    auto *charts = new QGridLayout(chartsPage);
    charts->setContentsMargins(0, 12, 0, 0);
    charts->setSpacing(16);
    dailyChartView_ = new QChartView(chartsPage);
    dailyChartView_->setObjectName(QStringLiteral("dailyFocusChartView"));
    dailyChartView_->setRenderHint(QPainter::Antialiasing);
    dailyChartView_->setMinimumHeight(230);
    dailyValueLabel_ = new QLabel(dailyChartView_->viewport());
    dailyValueLabel_->setObjectName(QStringLiteral("dailyFocusValueLabel"));
    dailyValueLabel_->setAlignment(Qt::AlignCenter);
    dailyValueLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    dailyValueLabel_->setAutoFillBackground(false);
    dailyValueLabel_->setFrameStyle(QFrame::NoFrame);
    dailyValueLabel_->setStyleSheet(QStringLiteral(
        "QLabel#dailyFocusValueLabel {"
        " color: #182230;"
        " background: transparent;"
        " border: none;"
        " padding: 0px;"
        " font-weight: 700;"
        "}"));
    dailyValueLabel_->hide();
    categoryChartView_ = new QChartView(chartsPage);
    categoryChartView_->setRenderHint(QPainter::Antialiasing);
    categoryChartView_->setMinimumHeight(230);
    projectChartView_ = new QChartView(chartsPage);
    projectChartView_->setRenderHint(QPainter::Antialiasing);
    projectChartView_->setMinimumHeight(230);
    charts->addWidget(dailyChartView_, 0, 0, 1, 2);
    charts->addWidget(categoryChartView_, 1, 0);
    charts->addWidget(projectChartView_, 1, 1);
    charts->setRowStretch(0, 1);
    charts->setRowStretch(1, 1);

    auto *recordsPage = new QWidget(tabs);
    auto *recordsPageLayout = new QVBoxLayout(recordsPage);
    recordsPageLayout->setContentsMargins(0, 12, 0, 0);
    auto *recentCard = new QFrame(recordsPage);
    recentCard->setObjectName(QStringLiteral("card"));
    recentCard->setProperty("statisticsSection", QStringLiteral("recentFocusCard"));
    auto *recentLayout = new QVBoxLayout(recentCard);
    recentLayout->setContentsMargins(18, 16, 18, 16);
    recentLayout->setSpacing(12);
    auto *recentTitle = new QLabel(QStringLiteral("最近专注记录"), recentCard);
    recentTitle->setObjectName(QStringLiteral("cardTitle"));
    auto *recentHint = new QLabel(
        QStringLiteral("仅显示最近 10 条专注记录，编号 1 表示最新记录。"),
        recentCard);
    recentHint->setObjectName(QStringLiteral("recentFocusHint"));
    recentHint->setProperty("muted", true);
    recentHint->setStyleSheet(QStringLiteral("color: #667085;"));
    recentSessions_ = new QTableWidget(0, 7, recentCard);
    recentSessions_->setObjectName(QStringLiteral("recentFocusSessions"));
    recentSessions_->setHorizontalHeaderLabels(
        {QStringLiteral("编号"), QStringLiteral("开始时间"), QStringLiteral("任务"),
         QStringLiteral("项目"), QStringLiteral("分类"),
         QStringLiteral("时长"), QStringLiteral("结果")});
    recentSessions_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentSessions_->setSelectionBehavior(QAbstractItemView::SelectRows);
    recentSessions_->setSelectionMode(QAbstractItemView::SingleSelection);
    recentSessions_->setAlternatingRowColors(true);
    recentSessions_->verticalHeader()->setVisible(false);
    recentSessions_->verticalHeader()->setMinimumSectionSize(36);
    recentSessions_->verticalHeader()->setDefaultSectionSize(36);
    recentSessions_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    recentSessions_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    recentSessions_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    recentSessions_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
    recentSessions_->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    recentSessions_->horizontalHeader()->setSectionsClickable(true);
    recentSessions_->horizontalHeader()->setSortIndicatorShown(true);
    recentSessions_->horizontalHeader()->setSortIndicator(1, Qt::DescendingOrder);
    recentSessions_->horizontalHeader()->setToolTip(
        QStringLiteral("点击任意列标题排序，再次点击可切换升序或降序"));
    recentSessions_->setSortingEnabled(true);
    recentSessions_->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
    enableClearSelectionOnBlankClick(recentSessions_);
    enableClearSelectionOnClick(recordsPage, recentSessions_);
    enableClearSelectionOnClick(recentCard, recentSessions_);
    enableClearSelectionOnClick(recentTitle, recentSessions_);
    enableClearSelectionOnClick(recentHint, recentSessions_);
    recentLayout->addWidget(recentTitle);
    recentLayout->addWidget(recentHint);
    recentLayout->addWidget(recentSessions_, 1);
    recordsPageLayout->addWidget(recentCard, 1);

    tabs->addTab(chartsPage, QStringLiteral("图表分析"));
    tabs->addTab(recordsPage, QStringLiteral("最近记录"));

    root->addLayout(summary);
    root->addWidget(tabs, 1);
}

void StatisticsPage::refresh()
{
    const DashboardMetrics metrics = AnalyticsRepository().dashboardMetrics();
    todayFocusValue_->setText(formatDuration(metrics.focusSecondsToday));
    weekFocusValue_->setText(formatDuration(metrics.focusSecondsLastSevenDays));
    completedValue_->setText(QString::number(metrics.completedToday));
    updateDailyChart();
    updateCategoryChart();
    updateProjectChart();
    updateRecentSessions();
}

void StatisticsPage::updateDailyChart()
{
    dailyValueLabel_->hide();
    const auto daily = AnalyticsRepository().lastSevenDays();
    auto *focusSet = new QBarSet(QStringLiteral("专注秒数"));
    focusSet->setObjectName(QStringLiteral("dailyFocusBarSet"));
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
    connect(focusSet, &QBarSet::hovered, focusSet,
            [this, chart, daily, maximum](bool hovered, int index) {
                if (!hovered || index < 0 || index >= daily.size()) {
                    dailyValueLabel_->hide();
                    dailyChartView_->setProperty("focusValueAnchor", QVariant());
                    return;
                }

                const DailyProductivity &day = daily.at(index);
                const QRectF plot = chart->plotArea();
                const qreal x = plot.left()
                    + plot.width() * (static_cast<qreal>(index) + 0.5)
                          / qMax(1, daily.size());
                const qreal ratio = static_cast<qreal>(day.focusSeconds)
                                    / qMax(1, maximum);
                const qreal y = plot.bottom() - plot.height() * ratio;
                const QPoint barTop = dailyChartView_->mapFromScene(
                    chart->mapToScene(QPointF(x, qMax(plot.top(), y))));

                dailyValueLabel_->setText(QString::number(day.focusSeconds));
                dailyValueLabel_->adjustSize();
                const QSize labelSize = dailyValueLabel_->size();
                const QRect viewportRect = dailyChartView_->viewport()->rect();
                const int left = qBound(
                    viewportRect.left(),
                    qRound(barTop.x() - labelSize.width() / 2.0),
                    qMax(viewportRect.left(),
                         viewportRect.right() - labelSize.width() + 1));
                const int top = qMax(viewportRect.top(),
                                     barTop.y() - labelSize.height() - 6);
                dailyValueLabel_->move(left, top);
                dailyValueLabel_->show();
                dailyValueLabel_->raise();
                dailyChartView_->setProperty("focusValueAnchor", barTop);
                dailyChartView_->setProperty("focusValueIndex", index);
            });
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
            slice->setBrush(QColor(QStringLiteral("#D0D5DD")));
        } else {
            slice->setBrush(QColor(categories.at(index).color));
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

void StatisticsPage::updateProjectChart()
{
    const auto projects = AnalyticsRepository().focusByProject();
    auto *series = new QPieSeries;
    for (const auto &project : projects) {
        series->append(project.name, project.focusSeconds);
    }
    if (projects.isEmpty()) {
        series->append(QStringLiteral("暂无数据"), 1);
    }
    const auto slices = series->slices();
    for (qsizetype index = 0; index < slices.size(); ++index) {
        auto *slice = slices.at(index);
        slice->setLabelVisible(true);
        if (projects.isEmpty()) {
            slice->setLabel(QStringLiteral("暂无数据"));
            slice->setBrush(QColor(QStringLiteral("#D0D5DD")));
        } else {
            slice->setBrush(QColor(projects.at(index).color));
            slice->setLabel(QStringLiteral("%1 %2% · %3")
                                .arg(projects.at(index).name)
                                .arg(slice->percentage() * 100.0, 0, 'f', 0)
                                .arg(formatDuration(projects.at(index).focusSeconds)));
        }
    }

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("近30天项目专注占比"));
    chart->legend()->setVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    projectChartView_->setChart(chart);
}

void StatisticsPage::updateRecentSessions()
{
    const auto sessions = AnalyticsRepository().recentFocusSessions();
    const int sortColumn =
        recentSessions_->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder sortOrder =
        recentSessions_->horizontalHeader()->sortIndicatorOrder();
    recentSessions_->setSortingEnabled(false);
    recentSessions_->setRowCount(sessions.size());
    recentSessions_->verticalHeader()->setSectionResizeMode(
        sessions.size() == 10 ? QHeaderView::Stretch : QHeaderView::Fixed);
    for (qsizetype row = 0; row < sessions.size(); ++row) {
        const auto &session = sessions.at(row);
        const QString result = session.completed ? QStringLiteral("已完成")
                                                 : QStringLiteral("已终止");
        const QStringList values{
            QString::number(row + 1), session.startedAt, session.taskName,
            session.projectName, session.categoryName,
            formatDuration(session.focusSeconds), result,
        };
        const QVariantList sortKeys{
            QVariant::fromValue(static_cast<int>(row + 1)),
            session.startedAt,
            session.taskName.toCaseFolded(),
            session.projectName.toCaseFolded(),
            session.categoryName.toCaseFolded(),
            session.focusSeconds,
            session.completed ? 1 : 0,
        };
        for (int column = 0; column < values.size(); ++column) {
            auto *item = new SortKeyTableWidgetItem(values.at(column),
                                                    sortKeys.at(column));
            item->setTextAlignment(Qt::AlignCenter);
            if (column == 3 && QColor(session.projectColor).isValid()) {
                item->setForeground(QColor(session.projectColor));
            } else if (column == 4
                       && QColor(session.categoryColor).isValid()) {
                item->setForeground(QColor(session.categoryColor));
            } else if (column == 6) {
                item->setForeground(
                    StatusColors::focusResult(session.completed));
                item->setToolTip(
                    QStringLiteral("专注结果：%1").arg(result));
            }
            recentSessions_->setItem(row, column, item);
        }
    }
    recentSessions_->setSortingEnabled(true);
    recentSessions_->sortItems(sortColumn >= 0 ? sortColumn : 1, sortOrder);
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
