#include "views/TaskDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

TaskDialog::TaskDialog(const QVector<LookupItem> &projects,
                       const QVector<LookupItem> &categories,
                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("新建任务"));
    setMinimumWidth(500);
    buildInterface(projects, categories);
}

void TaskDialog::buildInterface(const QVector<LookupItem> &projects,
                                const QVector<LookupItem> &categories)
{
    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(14);

    titleEdit_ = new QLineEdit(this);
    titleEdit_->setPlaceholderText(QStringLiteral("例如：完成工程实践需求分析"));
    titleEdit_->setMaxLength(120);

    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setPlaceholderText(QStringLiteral("记录详细要求、下一步行动或备注……"));
    descriptionEdit_->setMaximumHeight(110);

    projectCombo_ = new QComboBox(this);
    projectCombo_->addItem(QStringLiteral("无项目"), -1);
    for (const auto &project : projects) {
        projectCombo_->addItem(project.name, project.id);
    }

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(QStringLiteral("未分类"), -1);
    for (const auto &category : categories) {
        categoryCombo_->addItem(category.name, category.id);
    }

    importanceCombo_ = new QComboBox(this);
    importanceCombo_->addItem(QStringLiteral("1 - 很低"), 1);
    importanceCombo_->addItem(QStringLiteral("2 - 较低"), 2);
    importanceCombo_->addItem(QStringLiteral("3 - 普通"), 3);
    importanceCombo_->addItem(QStringLiteral("4 - 重要"), 4);
    importanceCombo_->addItem(QStringLiteral("5 - 紧急且重要"), 5);
    importanceCombo_->setCurrentIndex(2);

    dueEnabled_ = new QCheckBox(QStringLiteral("设置截止时间"), this);
    dueEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), this);
    dueEdit_->setCalendarPopup(true);
    dueEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    dueEdit_->setEnabled(false);
    connect(dueEnabled_, &QCheckBox::toggled, dueEdit_, &QWidget::setEnabled);

    auto *dueWidget = new QWidget(this);
    auto *dueLayout = new QVBoxLayout(dueWidget);
    dueLayout->setContentsMargins(0, 0, 0, 0);
    dueLayout->setSpacing(6);
    dueLayout->addWidget(dueEnabled_);
    dueLayout->addWidget(dueEdit_);

    estimatedMinutes_ = new QSpinBox(this);
    estimatedMinutes_->setRange(0, 1440);
    estimatedMinutes_->setValue(25);
    estimatedMinutes_->setSuffix(QStringLiteral(" 分钟"));

    form->addRow(QStringLiteral("任务标题："), titleEdit_);
    form->addRow(QStringLiteral("详细描述："), descriptionEdit_);
    form->addRow(QStringLiteral("所属项目："), projectCombo_);
    form->addRow(QStringLiteral("任务分类："), categoryCombo_);
    form->addRow(QStringLiteral("重要程度："), importanceCombo_);
    form->addRow(QStringLiteral("截止时间："), dueWidget);
    form->addRow(QStringLiteral("预计耗时："), estimatedMinutes_);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &TaskDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &TaskDialog::reject);

    root->addLayout(form);
    root->addSpacing(8);
    root->addWidget(buttons);
}

void TaskDialog::setTask(const Task &task)
{
    originalTask_ = task;
    setWindowTitle(task.id > 0 ? QStringLiteral("编辑任务")
                               : QStringLiteral("新建任务"));
    titleEdit_->setText(task.title);
    descriptionEdit_->setPlainText(task.description);
    selectId(projectCombo_, task.projectId);
    selectId(categoryCombo_, task.categoryId);
    selectId(importanceCombo_, task.importance);
    estimatedMinutes_->setValue(task.estimatedMinutes);
    dueEnabled_->setChecked(task.dueAt.isValid());
    if (task.dueAt.isValid()) {
        dueEdit_->setDateTime(task.dueAt);
    }
}

Task TaskDialog::task() const
{
    Task result = originalTask_;
    result.title = titleEdit_->text().trimmed();
    result.description = descriptionEdit_->toPlainText().trimmed();
    result.projectId = projectCombo_->currentData().toInt();
    result.categoryId = categoryCombo_->currentData().toInt();
    result.importance = importanceCombo_->currentData().toInt();
    result.dueAt = dueEnabled_->isChecked() ? dueEdit_->dateTime() : QDateTime();
    result.estimatedMinutes = estimatedMinutes_->value();
    return result;
}

void TaskDialog::accept()
{
    if (titleEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("无法保存"),
                             QStringLiteral("请输入任务标题。"));
        titleEdit_->setFocus();
        return;
    }
    QDialog::accept();
}

void TaskDialog::selectId(QComboBox *comboBox, int id)
{
    const int index = comboBox->findData(id);
    comboBox->setCurrentIndex(index >= 0 ? index : 0);
}
