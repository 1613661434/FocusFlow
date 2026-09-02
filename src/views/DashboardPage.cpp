#include "views/DashboardPage.h"

#include "repositories/AnalyticsRepository.h"
#include "repositories/TaskRepository.h"
#include "services/PriorityService.h"
#include "widgets/ClearSelectionOnBlankClick.h"
#include "widgets/ColoredComboBox.h"
#include "widgets/PriorityColors.h"

#include <QFrame>
#include <QAbstractItemView>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kAllLookups = -2;
constexpr int kProjectRole = Qt::UserRole + 1;
constexpr int kCategoryRole = Qt::UserRole + 2;
}

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
    focusValue_->setProperty("dashboardMetric", QStringLiteral("focusToday"));

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

    auto *filterLayout = new QHBoxLayout;
    filterLayout->setContentsMargins(0, 4, 0, 4);
    filterLayout->setSpacing(8);
    auto *projectLabel = new QLabel(QStringLiteral("项目筛选："), recommendationCard);
    projectFilter_ = new QComboBox(recommendationCard);
    projectFilter_->setObjectName(QStringLiteral("recommendationProjectFilter"));
    projectFilter_->setMinimumWidth(160);
    auto *categoryLabel = new QLabel(QStringLiteral("分类筛选："), recommendationCard);
    categoryFilter_ = new QComboBox(recommendationCard);
    categoryFilter_->setObjectName(QStringLiteral("recommendationCategoryFilter"));
    categoryFilter_->setMinimumWidth(160);
    ColoredComboBox::enableCurrentItemColor(projectFilter_);
    ColoredComboBox::enableCurrentItemColor(categoryFilter_);
    filterLayout->addWidget(projectLabel);
    filterLayout->addWidget(projectFilter_);
    filterLayout->addSpacing(12);
    filterLayout->addWidget(categoryLabel);
    filterLayout->addWidget(categoryFilter_);
    filterLayout->addStretch();

    recommendationContent_ = new QStackedWidget(recommendationCard);
    recommendationContent_->setObjectName(QStringLiteral("recommendationContent"));

    recommendations_ = new QListWidget(recommendationContent_);
    recommendations_->setObjectName(QStringLiteral("recommendationList"));
    recommendations_->setAlternatingRowColors(true);
    recommendations_->setFocusPolicy(Qt::StrongFocus);
    recommendations_->setSelectionMode(QAbstractItemView::SingleSelection);
    enableClearSelectionOnBlankClick(recommendations_);

    auto *emptyStatePage = new QWidget(recommendationContent_);
    auto *emptyStateLayout = new QVBoxLayout(emptyStatePage);
    emptyStateLayout->setContentsMargins(0, 0, 0, 0);
    emptyStateLayout->setSpacing(0);
    emptyStateLabel_ = new QLabel(
        QStringLiteral("暂无待办任务，可以好好休息一下。"),
        emptyStatePage);
    emptyStateLabel_->setObjectName(QStringLiteral("emptyStateLabel"));
    emptyStateLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    emptyStateLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    emptyStateLayout->addWidget(emptyStateLabel_);
    emptyStateLayout->addStretch(1);

    recommendationContent_->addWidget(recommendations_);
    recommendationContent_->addWidget(emptyStatePage);
    recommendationLayout->addWidget(title);
    recommendationLayout->addWidget(description);
    recommendationLayout->addLayout(filterLayout);
    recommendationLayout->addWidget(recommendationContent_, 1);

    connect(recommendations_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *item) {
                const int taskId = item->data(Qt::UserRole).toInt();
                if (taskId > 0) {
                    emit focusTaskRequested(taskId);
                }
            });
    connect(projectFilter_, &QComboBox::currentIndexChanged,
            this, [this] { refreshRecommendations(); });
    connect(categoryFilter_, &QComboBox::currentIndexChanged,
            this, [this] { refreshRecommendations(); });

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

    reloadRecommendationFilters();
    refreshRecommendations();
}

void DashboardPage::reloadRecommendationFilters()
{
    const int selectedProject = projectFilter_->currentData().isValid()
                                    ? projectFilter_->currentData().toInt()
                                    : kAllLookups;
    const int selectedCategory = categoryFilter_->currentData().isValid()
                                     ? categoryFilter_->currentData().toInt()
                                     : kAllLookups;
    const QSignalBlocker projectBlocker(projectFilter_);
    const QSignalBlocker categoryBlocker(categoryFilter_);

    TaskRepository repository;
    projectFilter_->clear();
    projectFilter_->addItem(QStringLiteral("全部项目"), kAllLookups);
    projectFilter_->addItem(QStringLiteral("无项目"), -1);
    for (const LookupItem &project : repository.projects()) {
        ColoredComboBox::addColoredItem(
            projectFilter_, project.name, project.id, QColor(project.color));
    }

    categoryFilter_->clear();
    categoryFilter_->addItem(QStringLiteral("全部分类"), kAllLookups);
    categoryFilter_->addItem(QStringLiteral("未分类"), -1);
    for (const LookupItem &category : repository.categories()) {
        ColoredComboBox::addColoredItem(
            categoryFilter_, category.name, category.id, QColor(category.color));
    }

    const int projectIndex = projectFilter_->findData(selectedProject);
    const int categoryIndex = categoryFilter_->findData(selectedCategory);
    projectFilter_->setCurrentIndex(projectIndex >= 0 ? projectIndex : 0);
    categoryFilter_->setCurrentIndex(categoryIndex >= 0 ? categoryIndex : 0);
    ColoredComboBox::applyCurrentItemColor(projectFilter_);
    ColoredComboBox::applyCurrentItemColor(categoryFilter_);
}

void DashboardPage::refreshRecommendations()
{
    if (projectFilter_ == nullptr || categoryFilter_ == nullptr) {
        return;
    }

    auto tasks = TaskRepository().findAll(TaskRepository::Filter::Recommended);
    const int selectedProject = projectFilter_->currentData().toInt();
    const int selectedCategory = categoryFilter_->currentData().toInt();
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [&](const Task &task) {
        const bool projectMismatch =
            selectedProject != kAllLookups && task.projectId != selectedProject;
        const bool categoryMismatch =
            selectedCategory != kAllLookups && task.categoryId != selectedCategory;
        return projectMismatch || categoryMismatch;
    }), tasks.end());
    std::stable_sort(tasks.begin(), tasks.end(), [](const Task &left, const Task &right) {
        return PriorityService::score(left) > PriorityService::score(right);
    });
    recommendations_->clear();
    const int count = qMin(6, tasks.size());
    const bool isEmpty = count == 0;
    recommendationContent_->setCurrentIndex(isEmpty ? 1 : 0);
    emptyStateLabel_->setText(
        selectedProject == kAllLookups && selectedCategory == kAllLookups
            ? QStringLiteral("暂无待办任务，可以好好休息一下。")
            : QStringLiteral("当前项目和分类下暂无待办任务。"));
    for (int index = 0; index < count; ++index) {
        const Task &task = tasks.at(index);
        const int score = PriorityService::score(task);
        const QString projectName = task.projectName.isEmpty()
                                        ? QStringLiteral("无项目")
                                        : task.projectName;
        const QString categoryName = task.categoryName.isEmpty()
                                         ? QStringLiteral("未分类")
                                         : task.categoryName;
        QString detail = QStringLiteral("项目：%1  ·  分类：%2  ·  推荐分 %3")
                             .arg(projectName,
                                  categoryName,
                                  QString::number(score));
        if (task.dueAt.isValid()) {
            detail += QStringLiteral("  ·  截止 %1")
                          .arg(task.dueAt.toString(QStringLiteral("MM-dd HH:mm")));
        }
        auto *item = new QListWidgetItem(recommendations_);
        item->setData(Qt::UserRole, task.id);
        item->setData(kProjectRole, task.projectId);
        item->setData(kCategoryRole, task.categoryId);
        item->setData(Qt::AccessibleTextRole,
                      QStringLiteral("%1，%2").arg(task.title, detail));
        item->setToolTip(
            QStringLiteral("推荐分：%1；颜色表示推荐程度区间").arg(score));
        item->setSizeHint(QSize(0, 60));

        auto *rowWidget = new QWidget(recommendations_);
        rowWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        rowWidget->setStyleSheet(QStringLiteral("background: transparent;"));
        auto *rowLayout = new QVBoxLayout(rowWidget);
        rowLayout->setContentsMargins(8, 5, 8, 5);
        rowLayout->setSpacing(3);
        auto *titleLabel = new QLabel(task.title, rowWidget);
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        titleLabel->setStyleSheet(
            QStringLiteral("color: #182230; background: transparent; "
                           "font-weight: 600;"));
        auto *detailLabel = new QLabel(rowWidget);
        detailLabel->setObjectName(QStringLiteral("recommendationScoreLine"));
        detailLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        detailLabel->setTextFormat(Qt::RichText);
        detailLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        const QColor projectColor = QColor(task.projectColor).isValid()
                                        ? QColor(task.projectColor)
                                        : QColor(QStringLiteral("#667085"));
        const QColor categoryColor = QColor(task.categoryColor).isValid()
                                         ? QColor(task.categoryColor)
                                         : QColor(QStringLiteral("#667085"));
        QString context =
            QStringLiteral("<span style=\"color:#667085;\">项目：</span>"
                           "<span style=\"color:%1;font-weight:600;\">%2</span>"
                           "<span style=\"color:#667085;\">"
                           "&nbsp;&nbsp;·&nbsp;&nbsp;分类：</span>"
                           "<span style=\"color:%3;font-weight:600;\">%4</span>")
                .arg(projectColor.name(), projectName.toHtmlEscaped(),
                     categoryColor.name(), categoryName.toHtmlEscaped());
        if (task.dueAt.isValid()) {
            context += QStringLiteral("<span style=\"color:#667085;\">"
                                      "&nbsp;&nbsp;·&nbsp;&nbsp;截止 %1</span>")
                           .arg(task.dueAt.toString(
                               QStringLiteral("MM-dd HH:mm")));
        }
        detailLabel->setText(
            QStringLiteral("<span style=\"color:%1;font-weight:600;\">"
                           "推荐分 %2</span>"
                           "<span style=\"color:#667085;\">"
                           "&nbsp;&nbsp;·&nbsp;&nbsp;</span>%3")
                .arg(PriorityColors::recommendation(score).name(),
                     QString::number(score),
                     context));
        rowLayout->addWidget(titleLabel);
        rowLayout->addWidget(detailLabel);
        recommendations_->setItemWidget(item, rowWidget);
    }
}

QString DashboardPage::formatDuration(int seconds)
{
    const int safeSeconds = qMax(0, seconds);
    const int hours = safeSeconds / 3600;
    const int minutes = (safeSeconds % 3600) / 60;
    const int remainingSeconds = safeSeconds % 60;
    if (hours > 0) {
        return QStringLiteral("%1时%2分%3秒")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
    }
    if (minutes > 0) {
        return QStringLiteral("%1分%2秒")
            .arg(minutes)
            .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1秒").arg(remainingSeconds);
}
