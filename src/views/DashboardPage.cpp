#include "views/DashboardPage.h"

#include "repositories/AnalyticsRepository.h"
#include "repositories/TaskRepository.h"
#include "services/PriorityService.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include <algorithm>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    buildInterface();
    refresh();
}

void DashboardPage::buildInterface()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(18);

    auto *metrics = new QGridLayout;
    metrics->setSpacing(14);
    metrics->addWidget(createMetricCard(QStringLiteral("待处理"), &pendingValue_), 0, 0);
    metrics->addWidget(createMetricCard(QStringLiteral("今日到期"), &todayValue_), 0, 1);
    metrics->addWidget(createMetricCard(QStringLiteral("已逾期"), &overdueValue_), 0, 2);
    metrics->addWidget(createMetricCard(QStringLiteral("今日完成"), &completedValue_), 0, 3);
    metrics->addWidget(createMetricCard(QStringLiteral("今日专注"), &focusValue_), 0, 4);

    auto *recommendationCard = new QFrame(this);
    recommendationCard->setObjectName(QStringLiteral("card"));
    auto *recommendationLayout = new QVBoxLayout(recommendationCard);
    recommendationLayout->setContentsMargins(24, 22, 24, 22);
    auto *title = new QLabel(QStringLiteral("优先建议"), recommendationCard);
    title->setObjectName(QStringLiteral("cardTitle"));
    auto *description = new QLabel(
        QStringLiteral("综合重要程度、截止时间、逾期情况和预计耗时排序。"),
        recommendationCard);
    description->setObjectName(QStringLiteral("mutedLabel"));
    recommendations_ = new QListWidget(recommendationCard);
    recommendations_->setObjectName(QStringLiteral("recommendationList"));
    recommendations_->setAlternatingRowColors(true);
    recommendations_->setFocusPolicy(Qt::NoFocus);
    recommendationLayout->addWidget(title);
    recommendationLayout->addWidget(description);
    recommendationLayout->addWidget(recommendations_, 1);

    root->addLayout(metrics);
    root->addWidget(recommendationCard, 1);
}

QWidget *DashboardPage::createMetricCard(const QString &title, QLabel **valueLabel)
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("mutedLabel"));
    *valueLabel = new QLabel(QStringLiteral("0"), card);
    (*valueLabel)->setObjectName(QStringLiteral("metricValue"));
    layout->addWidget(titleLabel);
    layout->addWidget(*valueLabel);
    return card;
}

void DashboardPage::refresh()
{
    const DashboardMetrics metrics = AnalyticsRepository().dashboardMetrics();
    pendingValue_->setText(QString::number(metrics.pendingTasks));
    todayValue_->setText(QString::number(metrics.dueToday));
    overdueValue_->setText(QString::number(metrics.overdueTasks));
    completedValue_->setText(QString::number(metrics.completedToday));
    focusValue_->setText(formatDuration(metrics.focusSecondsToday));

    auto tasks = TaskRepository().findAll(TaskRepository::Filter::Recommended);
    std::stable_sort(tasks.begin(), tasks.end(), [](const Task &left, const Task &right) {
        return PriorityService::score(left) > PriorityService::score(right);
    });
    recommendations_->clear();
    const int count = qMin(6, tasks.size());
    for (int index = 0; index < count; ++index) {
        const Task &task = tasks.at(index);
        QString detail = QStringLiteral("推荐分 %1").arg(PriorityService::score(task));
        if (task.dueAt.isValid()) {
            detail += QStringLiteral("  ·  截止 %1")
                          .arg(task.dueAt.toString(QStringLiteral("MM-dd HH:mm")));
        }
        auto *item = new QListWidgetItem(
            QStringLiteral("%1\n%2").arg(task.title, detail), recommendations_);
        item->setSizeHint(QSize(0, 54));
    }
    if (count == 0) {
        recommendations_->addItem(QStringLiteral("暂无待办任务，可以好好休息一下。"));
    }
}

QString DashboardPage::formatDuration(int seconds)
{
    const int minutes = qMax(0, seconds) / 60;
    if (minutes < 60) {
        return QStringLiteral("%1分").arg(minutes);
    }
    return QStringLiteral("%1时%2分").arg(minutes / 60).arg(minutes % 60);
}
