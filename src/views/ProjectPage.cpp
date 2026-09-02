#include "views/ProjectPage.h"

#include "widgets/ClearSelectionOnBlankClick.h"
#include "widgets/SortKeyTableWidgetItem.h"

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
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
constexpr int kColorRole = Qt::UserRole + 2;

class ColorSwatchDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem backgroundOption(option);
        initStyleOption(&backgroundOption, index);
        backgroundOption.text.clear();
        backgroundOption.icon = {};
        QStyledItemDelegate::paint(painter, backgroundOption, index);

        QColor color(index.data(kColorRole).toString());
        if (!color.isValid()) {
            color = QColor(QStringLiteral("#D0D5DD"));
        }
        const QPoint center = option.rect.center();
        const QRectF swatch(center.x() - 10.0,
                           center.y() - 10.0,
                           20.0,
                           20.0);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(QColor(QStringLiteral("#98A2B3")), 1));
        painter->setBrush(color);
        painter->drawRoundedRect(swatch.adjusted(1.0, 1.0, -1.0, -1.0),
                                 4.0,
                                 4.0);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setWidth(qMax(size.width(), 40));
        size.setHeight(qMax(size.height(), 32));
        return size;
    }
};

QString chooseColor(QWidget *parent, const QString &current)
{
    const QColor initial(current);
    QColorDialog dialog(initial.isValid() ? initial : QColor("#4F6EF7"),
                        parent);
    dialog.setWindowTitle(QStringLiteral("选择颜色"));
    dialog.setOption(QColorDialog::DontUseNativeDialog, true);
    if (auto *buttons = dialog.findChild<QDialogButtonBox *>()) {
        if (auto *ok = buttons->button(QDialogButtonBox::Ok)) {
            ok->setText(QStringLiteral("确定"));
        }
        if (auto *cancel = buttons->button(QDialogButtonBox::Cancel)) {
            cancel->setText(QStringLiteral("取消"));
        }
    }
    const QColor selected = dialog.exec() == QDialog::Accepted
                                ? dialog.selectedColor()
                                : QColor();
    return selected.isValid() ? selected.name(QColor::HexRgb).toUpper() : current;
}

QIcon colorSwatchIcon(const QString &colorText)
{
    QColor color(colorText);
    if (!color.isValid()) {
        color = QColor(QStringLiteral("#D0D5DD"));
    }
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#98A2B3")), 1));
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(1.0, 1.0, 18.0, 18.0), 4.0, 4.0);
    return QIcon(pixmap);
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
    auto *deleteProjectButton = new QPushButton(QStringLiteral("删除项目"), projectTab);
    deleteProjectButton->setObjectName(QStringLiteral("dangerButton"));
    projectButtons->addWidget(addProjectButton);
    projectButtons->addWidget(editProjectButton);
    projectButtons->addWidget(archiveProjectButton);
    projectButtons->addWidget(deleteProjectButton);
    projectButtons->addStretch();

    projectTable_ = new QTableWidget(projectTab);
    projectTable_->setObjectName(QStringLiteral("projectTable"));
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
    projectTable_->setIconSize(QSize(20, 20));
    projectTable_->verticalHeader()->setVisible(false);
    projectTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    projectTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    projectTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    projectTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    projectTable_->horizontalHeader()->setSectionsClickable(true);
    projectTable_->horizontalHeader()->setSortIndicatorShown(true);
    projectTable_->horizontalHeader()->setSortIndicator(3, Qt::AscendingOrder);
    projectTable_->horizontalHeader()->setToolTip(
        QStringLiteral("点击列标题排序；点击状态可切换归档项在前或在后"));
    projectTable_->setItemDelegateForColumn(
        2, new ColorSwatchDelegate(projectTable_));
    projectTable_->setSortingEnabled(true);
    enableClearSelectionOnBlankClick(projectTable_);
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
    categoryTable_->setObjectName(QStringLiteral("categoryTable"));
    categoryTable_->setColumnCount(2);
    categoryTable_->setHorizontalHeaderLabels({QStringLiteral("分类名称"),
                                               QStringLiteral("颜色")});
    categoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    categoryTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    categoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    categoryTable_->setAlternatingRowColors(true);
    categoryTable_->setShowGrid(false);
    categoryTable_->setIconSize(QSize(20, 20));
    categoryTable_->verticalHeader()->setVisible(false);
    categoryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    categoryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    categoryTable_->horizontalHeader()->setSectionsClickable(true);
    categoryTable_->horizontalHeader()->setSortIndicatorShown(true);
    categoryTable_->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
    categoryTable_->horizontalHeader()->setToolTip(
        QStringLiteral("点击列标题排序，再次点击可切换升序或降序"));
    categoryTable_->setItemDelegateForColumn(
        1, new ColorSwatchDelegate(categoryTable_));
    categoryTable_->setSortingEnabled(true);
    enableClearSelectionOnBlankClick(categoryTable_);
    categoryLayout->addLayout(categoryButtons);
    categoryLayout->addWidget(categoryTable_);

    tabs->addTab(projectTab, QStringLiteral("项目"));
    tabs->addTab(categoryTab, QStringLiteral("分类"));
    root->addWidget(tabs);

    connect(addProjectButton, &QPushButton::clicked, this, &ProjectPage::addProject);
    connect(editProjectButton, &QPushButton::clicked, this, &ProjectPage::editProject);
    connect(archiveProjectButton, &QPushButton::clicked, this, &ProjectPage::toggleProjectArchive);
    connect(deleteProjectButton, &QPushButton::clicked,
            this, &ProjectPage::deleteProject);
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
    const int projectSortColumn =
        projectTable_->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder projectSortOrder =
        projectTable_->horizontalHeader()->sortIndicatorOrder();
    projectTable_->setSortingEnabled(false);
    projectTable_->setRowCount(projects_.size());
    for (qsizetype row = 0; row < projects_.size(); ++row) {
        const auto &project = projects_.at(row);
        auto *name = new SortKeyTableWidgetItem(project.name);
        name->setData(Qt::UserRole, project.id);
        if (QColor(project.color).isValid()) {
            name->setForeground(QColor(project.color));
        }
        projectTable_->setItem(row, 0, name);
        projectTable_->setItem(
            row, 1, new SortKeyTableWidgetItem(project.description));
        auto *color = new SortKeyTableWidgetItem({}, project.color);
        color->setData(kColorRole, project.color);
        color->setToolTip(project.color);
        projectTable_->setItem(row, 2, color);
        projectTable_->setItem(row, 3,
                               new SortKeyTableWidgetItem(
                                   project.archived ? QStringLiteral("已归档")
                                                    : QStringLiteral("进行中"),
                                   project.archived ? 1 : 0));
        for (int column = 0; column < projectTable_->columnCount(); ++column) {
            if (auto *item = projectTable_->item(row, column)) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    projectTable_->setSortingEnabled(true);
    projectTable_->sortItems(projectSortColumn >= 0 ? projectSortColumn : 3,
                             projectSortOrder);

    categories_ = repository_.categories();
    const int categorySortColumn =
        categoryTable_->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder categorySortOrder =
        categoryTable_->horizontalHeader()->sortIndicatorOrder();
    categoryTable_->setSortingEnabled(false);
    categoryTable_->setRowCount(categories_.size());
    for (qsizetype row = 0; row < categories_.size(); ++row) {
        const auto &category = categories_.at(row);
        auto *name = new SortKeyTableWidgetItem(category.name);
        name->setData(Qt::UserRole, category.id);
        if (QColor(category.color).isValid()) {
            name->setForeground(QColor(category.color));
        }
        categoryTable_->setItem(row, 0, name);
        auto *color = new SortKeyTableWidgetItem({}, category.color);
        color->setData(kColorRole, category.color);
        color->setToolTip(category.color);
        categoryTable_->setItem(row, 1, color);
        for (int column = 0; column < categoryTable_->columnCount(); ++column) {
            if (auto *item = categoryTable_->item(row, column)) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    categoryTable_->setSortingEnabled(true);
    categoryTable_->sortItems(categorySortColumn >= 0 ? categorySortColumn : 0,
                              categorySortOrder);
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

void ProjectPage::deleteProject()
{
    const int index = selectedProjectIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("删除项目"),
                                 QStringLiteral("请先选择一个项目。"));
        return;
    }
    const Project &project = projects_.at(index);
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("永久删除项目"),
        QStringLiteral("确定永久删除“%1”吗？\n"
                       "此操作无法撤销；项目中的任务会保留并变为“无项目”。")
            .arg(project.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString error;
    if (!repository_.deleteProject(project.id, &error)) {
        showError(QStringLiteral("删除项目"), error);
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
    QString selectedColor = project.color;
    auto *colorButton = new QPushButton(QStringLiteral("选择颜色"), &dialog);
    colorButton->setIcon(colorSwatchIcon(selectedColor));
    colorButton->setIconSize(QSize(20, 20));
    connect(colorButton, &QPushButton::clicked, &dialog, [&, colorButton] {
        selectedColor = chooseColor(&dialog, selectedColor);
        colorButton->setIcon(colorSwatchIcon(selectedColor));
    });
    form->addRow(QStringLiteral("项目名称："), name);
    form->addRow(QStringLiteral("详细说明："), description);
    form->addRow(QStringLiteral("项目颜色："), colorButton);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
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
    project.color = selectedColor;
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
    if (row < 0) {
        return -1;
    }
    const QTableWidgetItem *name = projectTable_->item(row, 0);
    if (name == nullptr) {
        return -1;
    }
    const int projectId = name->data(Qt::UserRole).toInt();
    for (qsizetype index = 0; index < projects_.size(); ++index) {
        if (projects_.at(index).id == projectId) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int ProjectPage::selectedCategoryIndex() const
{
    const int row = categoryTable_->currentRow();
    if (row < 0) {
        return -1;
    }
    const QTableWidgetItem *name = categoryTable_->item(row, 0);
    if (name == nullptr) {
        return -1;
    }
    const int categoryId = name->data(Qt::UserRole).toInt();
    for (qsizetype index = 0; index < categories_.size(); ++index) {
        if (categories_.at(index).id == categoryId) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void ProjectPage::showError(const QString &action, const QString &details)
{
    QMessageBox::critical(this, QStringLiteral("操作失败"),
                          QStringLiteral("%1失败：\n%2").arg(action, details));
}
