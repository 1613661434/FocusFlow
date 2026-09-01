#include "views/ProjectPage.h"

#include <QAbstractItemView>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
QString chooseColor(QWidget *parent, const QString &current)
{
    const QColor initial(current);
    const QColor selected = QColorDialog::getColor(initial.isValid() ? initial : QColor("#4F6EF7"),
                                                    parent,
                                                    QStringLiteral("选择颜色"));
    return selected.isValid() ? selected.name(QColor::HexRgb).toUpper() : current;
}
}

ProjectPage::ProjectPage(QWidget *parent)
    : QWidget(parent)
{
    buildInterface();
    refresh();
}

void ProjectPage::buildInterface()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *tabs = new QTabWidget(this);
    auto *projectTab = new QWidget(tabs);
    auto *projectLayout = new QVBoxLayout(projectTab);
    projectLayout->setContentsMargins(12, 16, 12, 12);
    auto *projectButtons = new QHBoxLayout;
    auto *addProjectButton = new QPushButton(QStringLiteral("+ 新建项目"), projectTab);
    addProjectButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editProjectButton = new QPushButton(QStringLiteral("编辑"), projectTab);
    auto *archiveProjectButton = new QPushButton(QStringLiteral("归档 / 恢复"), projectTab);
    projectButtons->addWidget(addProjectButton);
    projectButtons->addWidget(editProjectButton);
    projectButtons->addWidget(archiveProjectButton);
    projectButtons->addStretch();

    projectTable_ = new QTableWidget(projectTab);
    projectTable_->setColumnCount(4);
    projectTable_->setHorizontalHeaderLabels({
        QStringLiteral("项目名称"),
        QStringLiteral("详细说明"),
        QStringLiteral("颜色"),
        QStringLiteral("状态"),
    });
    projectTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    projectTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    projectTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectTable_->setAlternatingRowColors(true);
    projectTable_->setShowGrid(false);
    projectTable_->verticalHeader()->setVisible(false);
    projectTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    projectTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    projectTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    projectTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    projectLayout->addLayout(projectButtons);
    projectLayout->addWidget(projectTable_);

    auto *categoryTab = new QWidget(tabs);
    auto *categoryLayout = new QVBoxLayout(categoryTab);
    categoryLayout->setContentsMargins(12, 16, 12, 12);
    auto *categoryButtons = new QHBoxLayout;
    auto *addCategoryButton = new QPushButton(QStringLiteral("+ 新建分类"), categoryTab);
    addCategoryButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editCategoryButton = new QPushButton(QStringLiteral("编辑"), categoryTab);
    auto *deleteCategoryButton = new QPushButton(QStringLiteral("删除"), categoryTab);
    deleteCategoryButton->setObjectName(QStringLiteral("dangerButton"));
    categoryButtons->addWidget(addCategoryButton);
    categoryButtons->addWidget(editCategoryButton);
    categoryButtons->addWidget(deleteCategoryButton);
    categoryButtons->addStretch();

    categoryTable_ = new QTableWidget(categoryTab);
    categoryTable_->setColumnCount(2);
    categoryTable_->setHorizontalHeaderLabels({QStringLiteral("分类名称"),
                                               QStringLiteral("颜色")});
    categoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    categoryTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    categoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    categoryTable_->setAlternatingRowColors(true);
    categoryTable_->setShowGrid(false);
    categoryTable_->verticalHeader()->setVisible(false);
    categoryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    categoryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    categoryLayout->addLayout(categoryButtons);
    categoryLayout->addWidget(categoryTable_);

    tabs->addTab(projectTab, QStringLiteral("项目"));
    tabs->addTab(categoryTab, QStringLiteral("分类"));
    root->addWidget(tabs);

    connect(addProjectButton, &QPushButton::clicked, this, &ProjectPage::addProject);
    connect(editProjectButton, &QPushButton::clicked, this, &ProjectPage::editProject);
    connect(archiveProjectButton, &QPushButton::clicked, this, &ProjectPage::toggleProjectArchive);
    connect(addCategoryButton, &QPushButton::clicked, this, &ProjectPage::addCategory);
    connect(editCategoryButton, &QPushButton::clicked, this, &ProjectPage::editCategory);
    connect(deleteCategoryButton, &QPushButton::clicked, this, &ProjectPage::deleteCategory);
    connect(projectTable_, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { editProject(); });
    connect(categoryTable_, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { editCategory(); });
}

void ProjectPage::refresh()
{
    projects_ = repository_.projects(true);
    projectTable_->setRowCount(projects_.size());
    for (qsizetype row = 0; row < projects_.size(); ++row) {
        const auto &project = projects_.at(row);
        auto *name = new QTableWidgetItem(project.name);
        name->setData(Qt::UserRole, project.id);
        projectTable_->setItem(row, 0, name);
        projectTable_->setItem(row, 1, new QTableWidgetItem(project.description));
        auto *color = new QTableWidgetItem(project.color);
        color->setForeground(QColor(project.color));
        projectTable_->setItem(row, 2, color);
        projectTable_->setItem(row, 3,
                               new QTableWidgetItem(project.archived
                                                        ? QStringLiteral("已归档")
                                                        : QStringLiteral("进行中")));
    }

    categories_ = repository_.categories();
    categoryTable_->setRowCount(categories_.size());
    for (qsizetype row = 0; row < categories_.size(); ++row) {
        const auto &category = categories_.at(row);
        auto *name = new QTableWidgetItem(category.name);
        name->setData(Qt::UserRole, category.id);
        categoryTable_->setItem(row, 0, name);
        auto *color = new QTableWidgetItem(category.color);
        color->setForeground(QColor(category.color));
        categoryTable_->setItem(row, 1, color);
    }
}

void ProjectPage::addProject()
{
    Project project;
    if (!editProjectValues(project)) {
        return;
    }
    QString error;
    if (!repository_.saveProject(project, &error)) {
        showError(QStringLiteral("新建项目"), error);
        return;
    }
    refresh();
    emit lookupsChanged();
}

void ProjectPage::editProject()
{
    const int index = selectedProjectIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("编辑项目"),
                                 QStringLiteral("请先选择一个项目。"));
        return;
    }
    Project project = projects_.at(index);
    if (!editProjectValues(project)) {
        return;
    }
    QString error;
    if (!repository_.saveProject(project, &error)) {
        showError(QStringLiteral("编辑项目"), error);
        return;
    }
    refresh();
    emit lookupsChanged();
}

void ProjectPage::toggleProjectArchive()
{
    const int index = selectedProjectIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("归档项目"),
                                 QStringLiteral("请先选择一个项目。"));
        return;
    }
    const Project &project = projects_.at(index);
    QString error;
    if (!repository_.setProjectArchived(project.id, !project.archived, &error)) {
        showError(QStringLiteral("更新项目状态"), error);
        return;
    }
    refresh();
    emit lookupsChanged();
}

void ProjectPage::addCategory()
{
    LookupItem category;
    category.color = QStringLiteral("#4F6EF7");
    if (!editCategoryValues(category)) {
        return;
    }
    QString error;
    if (!repository_.saveCategory(category, &error)) {
        showError(QStringLiteral("新建分类"), error);
        return;
    }
    refresh();
    emit lookupsChanged();
}

void ProjectPage::editCategory()
{
    const int index = selectedCategoryIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("编辑分类"),
                                 QStringLiteral("请先选择一个分类。"));
        return;
    }
    LookupItem category = categories_.at(index);
    if (!editCategoryValues(category)) {
        return;
    }
    QString error;
    if (!repository_.saveCategory(category, &error)) {
        showError(QStringLiteral("编辑分类"), error);
        return;
    }
    refresh();
    emit lookupsChanged();
}

void ProjectPage::deleteCategory()
{
    const int index = selectedCategoryIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("删除分类"),
                                 QStringLiteral("请先选择一个分类。"));
        return;
    }
    const LookupItem &category = categories_.at(index);
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("删除分类"),
        QStringLiteral("确定删除“%1”吗？相关任务将变为未分类。")
            .arg(category.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString error;
    if (!repository_.deleteCategory(category.id, &error)) {
        showError(QStringLiteral("删除分类"), error);
        return;
    }
    refresh();
    emit lookupsChanged();
}

bool ProjectPage::editProjectValues(Project &project)
{
    QDialog dialog(this);
    dialog.setWindowTitle(project.id > 0 ? QStringLiteral("编辑项目")
                                         : QStringLiteral("新建项目"));
    dialog.setMinimumWidth(460);
    auto *root = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *name = new QLineEdit(project.name, &dialog);
    auto *description = new QTextEdit(&dialog);
    description->setPlainText(project.description);
    description->setMaximumHeight(100);
    auto *colorButton = new QPushButton(project.color, &dialog);
    connect(colorButton, &QPushButton::clicked, &dialog, [&, colorButton] {
        const QString color = chooseColor(&dialog, colorButton->text());
        colorButton->setText(color);
        colorButton->setStyleSheet(QStringLiteral("color: %1;").arg(color));
    });
    form->addRow(QStringLiteral("项目名称："), name);
    form->addRow(QStringLiteral("详细说明："), description);
    form->addRow(QStringLiteral("项目颜色："), colorButton);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, QStringLiteral("无法保存"),
                                 QStringLiteral("请输入项目名称。"));
            return;
        }
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    root->addLayout(form);
    root->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    project.name = name->text().trimmed();
    project.description = description->toPlainText().trimmed();
    project.color = colorButton->text();
    return true;
}

bool ProjectPage::editCategoryValues(LookupItem &category)
{
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this,
        category.id > 0 ? QStringLiteral("编辑分类") : QStringLiteral("新建分类"),
        QStringLiteral("分类名称："),
        QLineEdit::Normal,
        category.name,
        &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return false;
    }
    category.name = name;
    category.color = chooseColor(this, category.color);
    return true;
}

int ProjectPage::selectedProjectIndex() const
{
    const int row = projectTable_->currentRow();
    return row >= 0 && row < projects_.size() ? row : -1;
}

int ProjectPage::selectedCategoryIndex() const
{
    const int row = categoryTable_->currentRow();
    return row >= 0 && row < categories_.size() ? row : -1;
}

void ProjectPage::showError(const QString &action, const QString &details)
{
    QMessageBox::critical(this, QStringLiteral("操作失败"),
                          QStringLiteral("%1失败：\n%2").arg(action, details));
}
