#include "views/TaskPage.h"

#include "views/TaskDialog.h"
#include "services/PriorityService.h"
#include "widgets/ClearSelectionOnBlankClick.h"
#include "widgets/PriorityColors.h"
#include "widgets/SortKeyTableWidgetItem.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace {
enum Column {
    TitleColumn,
    DescriptionColumn,
    ProjectColumn,
    CategoryColumn,
    DueColumn,
    ImportanceColumn,
    EstimateColumn,
    ScoreColumn,
    StatusColumn,
    ColumnCount,
};
}

TaskPage::TaskPage(QWidget *parent)
    : QWidget(parent)
{
    buildInterface();
    refresh();
}

void TaskPage::buildInterface()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(10);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(QStringLiteral("搜索任务、项目或分类……"));
    searchEdit_->setClearButtonEnabled(true);
    searchEdit_->setMinimumWidth(280);

    filterCombo_ = new QComboBox(this);
    filterCombo_->addItem(QStringLiteral("全部任务"),
                          static_cast<int>(TaskRepository::Filter::All));
    filterCombo_->addItem(QStringLiteral("智能推荐"),
                          static_cast<int>(TaskRepository::Filter::Recommended));
    filterCombo_->addItem(QStringLiteral("今日任务"),
                          static_cast<int>(TaskRepository::Filter::Today));
    filterCombo_->addItem(QStringLiteral("未来7天"),
                          static_cast<int>(TaskRepository::Filter::ThisWeek));
    filterCombo_->addItem(QStringLiteral("已经逾期"),
                          static_cast<int>(TaskRepository::Filter::Overdue));
    filterCombo_->addItem(QStringLiteral("已完成"),
                          static_cast<int>(TaskRepository::Filter::Completed));

    auto *addButton = new QPushButton(QStringLiteral("+ 新建任务"), this);
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"), this);
    completeButton_ = new QPushButton(QStringLiteral("完成"), this);
    auto *deleteButton = new QPushButton(QStringLiteral("删除"), this);
    deleteButton->setObjectName(QStringLiteral("dangerButton"));

    toolbar->addWidget(searchEdit_, 1);
    toolbar->addWidget(filterCombo_);
    toolbar->addSpacing(8);
    toolbar->addWidget(addButton);
    toolbar->addWidget(editButton);
    toolbar->addWidget(completeButton_);
    toolbar->addWidget(deleteButton);

    table_ = new QTableWidget(this);
    table_->setObjectName(QStringLiteral("taskTable"));
    table_->setColumnCount(ColumnCount);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("任务"),
        QStringLiteral("详细描述"),
        QStringLiteral("项目"),
        QStringLiteral("分类"),
        QStringLiteral("截止时间"),
        QStringLiteral("重要度"),
        QStringLiteral("预计"),
        QStringLiteral("推荐分"),
        QStringLiteral("状态"),
    });
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(false);
    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(44);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(TitleColumn, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(DescriptionColumn, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(ProjectColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(CategoryColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(DueColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(ImportanceColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(EstimateColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(ScoreColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(StatusColumn, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionsClickable(true);
    table_->horizontalHeader()->setSortIndicatorShown(true);
    table_->horizontalHeader()->setSortIndicator(ImportanceColumn,
                                                 Qt::DescendingOrder);
    table_->horizontalHeader()->setToolTip(
        QStringLiteral("点击列标题排序，再次点击可切换升序或降序"));
    table_->setSortingEnabled(true);
    enableClearSelectionOnBlankClick(table_);

    summaryLabel_ = new QLabel(this);
    summaryLabel_->setObjectName(QStringLiteral("mutedLabel"));

    root->addLayout(toolbar);
    root->addWidget(table_, 1);
    root->addWidget(summaryLabel_);

    connect(addButton, &QPushButton::clicked, this, &TaskPage::addTask);
    connect(editButton, &QPushButton::clicked, this, &TaskPage::editSelectedTask);
    connect(completeButton_, &QPushButton::clicked,
            this, &TaskPage::toggleSelectedTask);
    connect(deleteButton, &QPushButton::clicked,
            this, &TaskPage::deleteSelectedTask);
    connect(searchEdit_, &QLineEdit::textChanged,
            this, &TaskPage::refreshForFilter);
    connect(filterCombo_, &QComboBox::currentIndexChanged,
            this, &TaskPage::refreshForFilter);
    connect(table_, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { editSelectedTask(); });
    connect(table_, &QTableWidget::itemSelectionChanged, this, [this] {
        const int index = selectedTaskIndex();
        if (index < 0) {
            completeButton_->setText(QStringLiteral("完成"));
            return;
        }
        completeButton_->setText(tasks_.at(index).status == QStringLiteral("completed")
                                     ? QStringLiteral("重新打开")
                                     : QStringLiteral("完成"));
    });
}

void TaskPage::refresh()
{
    const auto filter = static_cast<TaskRepository::Filter>(
        filterCombo_->currentData().toInt());
    tasks_ = repository_.findAll(filter, searchEdit_->text());
    if (filter == TaskRepository::Filter::Recommended) {
        std::stable_sort(tasks_.begin(), tasks_.end(), [](const Task &left, const Task &right) {
            return PriorityService::score(left) > PriorityService::score(right);
        });
    }

    const int sortColumn = table_->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder sortOrder =
        table_->horizontalHeader()->sortIndicatorOrder();
    table_->setSortingEnabled(false);
    table_->setRowCount(tasks_.size());
    for (qsizetype row = 0; row < tasks_.size(); ++row) {
        const auto &task = tasks_.at(row);
        auto *title = new SortKeyTableWidgetItem(task.title);
        title->setData(Qt::UserRole, task.id);
        if (task.status == QStringLiteral("completed")) {
            QFont font = title->font();
            font.setStrikeOut(true);
            title->setFont(font);
            title->setForeground(QColor(QStringLiteral("#8A94A6")));
        }

        table_->setItem(row, TitleColumn, title);
        const bool hasDescription = !task.description.trimmed().isEmpty();
        auto *description = new SortKeyTableWidgetItem(
            hasDescription ? task.description : QStringLiteral("—"),
            task.description.toCaseFolded());
        description->setToolTip(hasDescription ? task.description
                                                : QStringLiteral("暂无详细描述"));
        if (!hasDescription) {
            description->setTextAlignment(Qt::AlignCenter);
        }
        table_->setItem(row, DescriptionColumn, description);

        const bool hasProject = !task.projectName.isEmpty();
        auto *project = new SortKeyTableWidgetItem(
            hasProject ? task.projectName : QStringLiteral("—"),
            task.projectName.toCaseFolded());
        if (!hasProject) {
            project->setTextAlignment(Qt::AlignCenter);
            project->setToolTip(QStringLiteral("未选择项目"));
        }
        if (hasProject && QColor(task.projectColor).isValid()) {
            project->setForeground(QColor(task.projectColor));
        }
        table_->setItem(row, ProjectColumn, project);
        auto *category = new SortKeyTableWidgetItem(
            task.categoryName.isEmpty() ? QStringLiteral("未分类")
                                        : task.categoryName,
            task.categoryName.toCaseFolded());
        if (!task.categoryName.isEmpty()
            && QColor(task.categoryColor).isValid()) {
            category->setForeground(QColor(task.categoryColor));
        }
        table_->setItem(row, CategoryColumn, category);

        QString dueText = QStringLiteral("无截止时间");
        bool overdue = false;
        if (task.dueAt.isValid()) {
            dueText = task.dueAt.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
            if (task.status != QStringLiteral("completed")
                && task.dueAt < QDateTime::currentDateTime()) {
                overdue = true;
                dueText = QStringLiteral("已逾期 ") + dueText;
            }
        }
        const qint64 dueSortKey = task.dueAt.isValid()
                                      ? task.dueAt.toMSecsSinceEpoch()
                                      : std::numeric_limits<qint64>::max();
        auto *dueItem = new SortKeyTableWidgetItem(dueText, dueSortKey);
        if (overdue) {
            dueItem->setForeground(QColor(QStringLiteral("#B42318")));
            dueItem->setToolTip(
                QStringLiteral("该任务已超过截止时间：%1")
                    .arg(task.dueAt.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
        }
        table_->setItem(row, DueColumn, dueItem);
        auto *importanceItem = new SortKeyTableWidgetItem(
            importanceText(task.importance), task.importance);
        importanceItem->setForeground(PriorityColors::importance(task.importance));
        importanceItem->setToolTip(
            QStringLiteral("重要程度：%1 / 5").arg(task.importance));
        table_->setItem(row, ImportanceColumn, importanceItem);
        table_->setItem(row, EstimateColumn,
                        new SortKeyTableWidgetItem(
                            QStringLiteral("%1 分钟").arg(task.estimatedMinutes),
                            task.estimatedMinutes));
        const int score = PriorityService::score(task);
        auto *scoreItem = new SortKeyTableWidgetItem(QString::number(score), score);
        scoreItem->setForeground(PriorityColors::recommendation(score));
        scoreItem->setToolTip(
            QStringLiteral("推荐分：%1；颜色表示推荐程度区间").arg(score));
        table_->setItem(row, ScoreColumn, scoreItem);
        int statusSortKey = 0;
        if (task.status == QStringLiteral("in_progress")) {
            statusSortKey = 1;
        } else if (task.status == QStringLiteral("completed")) {
            statusSortKey = 2;
        } else if (task.status == QStringLiteral("cancelled")) {
            statusSortKey = 3;
        }
        table_->setItem(row, StatusColumn,
                        new SortKeyTableWidgetItem(
                            statusText(task.status), statusSortKey));
        for (int column = 0; column < ColumnCount; ++column) {
            if (auto *item = table_->item(row, column)) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    table_->setSortingEnabled(true);
    table_->sortItems(sortColumn >= 0 ? sortColumn : ImportanceColumn, sortOrder);

    summaryLabel_->setText(QStringLiteral("当前显示 %1 项任务").arg(tasks_.size()));
    completeButton_->setText(QStringLiteral("完成"));
}

void TaskPage::addTask()
{
    TaskDialog dialog(repository_.projects(), repository_.categories(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Task task = dialog.task();
    QString error;
    if (!repository_.save(task, &error)) {
        showRepositoryError(QStringLiteral("新建任务"), error);
        return;
    }
    refresh();
    emit tasksChanged();
}

void TaskPage::editSelectedTask()
{
    const int id = selectedTaskId();
    if (id < 0) {
        QMessageBox::information(this,
                                 QStringLiteral("编辑任务"),
                                 QStringLiteral("请先选择一项任务。"));
        return;
    }

    Task task = repository_.findById(id);
    if (task.id < 0) {
        showRepositoryError(QStringLiteral("读取任务"),
                            QStringLiteral("任务不存在或已被删除。"));
        return;
    }

    TaskDialog dialog(repository_.projects(), repository_.categories(), this);
    dialog.setTask(task);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    task = dialog.task();
    QString error;
    if (!repository_.save(task, &error)) {
        showRepositoryError(QStringLiteral("保存任务"), error);
        return;
    }
    refresh();
    emit tasksChanged();
}

void TaskPage::toggleSelectedTask()
{
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this,
                                 QStringLiteral("任务状态"),
                                 QStringLiteral("请先选择一项任务。"));
        return;
    }

    const auto &task = tasks_.at(index);
    const bool completed = task.status != QStringLiteral("completed");
    QString error;
    if (!repository_.setCompleted(task.id, completed, &error)) {
        showRepositoryError(QStringLiteral("更新任务状态"), error);
        return;
    }
    refresh();
    emit tasksChanged();
}

void TaskPage::deleteSelectedTask()
{
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this,
                                 QStringLiteral("删除任务"),
                                 QStringLiteral("请先选择一项任务。"));
        return;
    }

    const auto &task = tasks_.at(index);
    const auto choice = QMessageBox::question(
        this,
        QStringLiteral("永久删除任务"),
        QStringLiteral("确定永久删除“%1”吗？\n"
                       "此操作无法撤销；已有专注记录会保留，但将解除任务关联。")
            .arg(task.title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!repository_.deleteTask(task.id, &error)) {
        showRepositoryError(QStringLiteral("删除任务"), error);
        return;
    }
    refresh();
    emit tasksChanged();
}

void TaskPage::refreshForFilter()
{
    refresh();
}

int TaskPage::selectedTaskId() const
{
    const int index = selectedTaskIndex();
    return index >= 0 ? tasks_.at(index).id : -1;
}

int TaskPage::selectedTaskIndex() const
{
    const int row = table_->currentRow();
    if (row < 0) {
        return -1;
    }
    const QTableWidgetItem *title = table_->item(row, TitleColumn);
    if (title == nullptr) {
        return -1;
    }
    const int taskId = title->data(Qt::UserRole).toInt();
    for (qsizetype index = 0; index < tasks_.size(); ++index) {
        if (tasks_.at(index).id == taskId) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void TaskPage::showRepositoryError(const QString &action, const QString &details)
{
    QMessageBox::critical(this,
                          QStringLiteral("操作失败"),
                          QStringLiteral("%1失败：\n%2").arg(action, details));
}

QString TaskPage::statusText(const QString &status)
{
    if (status == QStringLiteral("completed")) {
        return QStringLiteral("已完成");
    }
    if (status == QStringLiteral("in_progress")) {
        return QStringLiteral("进行中");
    }
    if (status == QStringLiteral("cancelled")) {
        return QStringLiteral("已取消");
    }
    return QStringLiteral("待处理");
}

QString TaskPage::importanceText(int importance)
{
    return QString(importance, QChar(0x2605));
}
