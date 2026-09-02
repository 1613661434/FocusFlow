#include "views/TaskDialog.h"

#include "widgets/ChineseCalendarWidget.h"
#include "widgets/ColoredComboBox.h"
#include "widgets/FocusAwareSpinBox.h"
#include "widgets/PriorityColors.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTime>
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
    projectCombo_->setObjectName(QStringLiteral("taskProjectCombo"));
    projectCombo_->addItem(QStringLiteral("无项目"), -1);
    for (const auto &project : projects) {
        ColoredComboBox::addColoredItem(
            projectCombo_, project.name, project.id, QColor(project.color));
    }
    ColoredComboBox::enableCurrentItemColor(projectCombo_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->setObjectName(QStringLiteral("taskCategoryCombo"));
    categoryCombo_->addItem(QStringLiteral("未分类"), -1);
    for (const auto &category : categories) {
        ColoredComboBox::addColoredItem(
            categoryCombo_, category.name, category.id, QColor(category.color));
    }
    ColoredComboBox::enableCurrentItemColor(categoryCombo_);

    importanceCombo_ = new QComboBox(this);
    importanceCombo_->setObjectName(QStringLiteral("taskImportanceCombo"));
    const QStringList importanceNames{
        QStringLiteral("很低"), QStringLiteral("较低"),
        QStringLiteral("普通"), QStringLiteral("重要"),
        QStringLiteral("紧急且重要")};
    for (int level = 1; level <= 5; ++level) {
        ColoredComboBox::addColoredItem(
            importanceCombo_,
            QStringLiteral("%1  %2（%3级）")
                .arg(QString(level, QChar(0x2605)),
                     importanceNames.at(level - 1),
                     QString::number(level)),
            level,
            PriorityColors::importance(level));
    }
    importanceCombo_->setCurrentIndex(2);
    ColoredComboBox::enableCurrentItemColor(importanceCombo_);

    const QDateTime defaultDue(QDate::currentDate(), QTime(23, 59));
    dueEnabled_ = new QCheckBox(QStringLiteral("设置截止时间"), this);
    dueEnabled_->setObjectName(QStringLiteral("dueEnabled"));
    dueEnabled_->setChecked(true);

    dueEdit_ = new QDateTimeEdit(defaultDue, this);
    dueEdit_->setObjectName(QStringLiteral("dueDateEdit"));
    dueEdit_->setLocale(QLocale(QLocale::Chinese, QLocale::China));
    dueEdit_->setCalendarPopup(true);
    dueEdit_->setCalendarWidget(new ChineseCalendarWidget(dueEdit_));
    dueEdit_->setDisplayFormat(QStringLiteral("yyyy年MM月dd日"));

    dueHour_ = new FocusAwareSpinBox(this);
    dueHour_->setObjectName(QStringLiteral("dueHourSpin"));
    dueHour_->setRange(0, 23);
    dueHour_->setValue(defaultDue.time().hour());
    dueHour_->setSuffix(QStringLiteral(" 时"));
    dueHour_->setMinimumWidth(88);
    dueHour_->setToolTip(QStringLiteral("截止小时（24 小时制）"));

    dueMinute_ = new FocusAwareSpinBox(this);
    dueMinute_->setObjectName(QStringLiteral("dueMinuteSpin"));
    dueMinute_->setRange(0, 59);
    dueMinute_->setValue(defaultDue.time().minute());
    dueMinute_->setSuffix(QStringLiteral(" 分"));
    dueMinute_->setMinimumWidth(88);
    dueMinute_->setToolTip(QStringLiteral("截止分钟"));

    auto *dueTimeLayout = new QHBoxLayout;
    dueTimeLayout->setContentsMargins(0, 0, 0, 0);
    dueTimeLayout->setSpacing(8);
    dueTimeLayout->addWidget(dueEdit_, 1);
    dueTimeLayout->addWidget(dueHour_);
    dueTimeLayout->addWidget(dueMinute_);

    connect(dueEnabled_, &QCheckBox::toggled, this, [this](bool enabled) {
        dueEdit_->setEnabled(enabled);
        dueHour_->setEnabled(enabled);
        dueMinute_->setEnabled(enabled);
    });

    auto *dueWidget = new QWidget(this);
    auto *dueLayout = new QVBoxLayout(dueWidget);
    dueLayout->setContentsMargins(0, 0, 0, 0);
    dueLayout->setSpacing(6);
    dueLayout->addWidget(dueEnabled_);
    dueLayout->addLayout(dueTimeLayout);

    estimatedMinutes_ = new FocusAwareSpinBox(this);
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
        dueEdit_->setDate(task.dueAt.date());
        dueHour_->setValue(task.dueAt.time().hour());
        dueMinute_->setValue(task.dueAt.time().minute());
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
    result.dueAt = dueEnabled_->isChecked()
                       ? QDateTime(dueEdit_->date(),
                                   QTime(dueHour_->value(), dueMinute_->value()))
                       : QDateTime();
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
    ColoredComboBox::applyCurrentItemColor(comboBox);
}
