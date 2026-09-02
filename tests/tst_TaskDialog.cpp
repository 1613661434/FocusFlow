#include "views/TaskDialog.h"
#include "widgets/PriorityColors.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QTest>

class TaskDialogTests final : public QObject
{
    Q_OBJECT

private slots:
    void newTaskDefaultsToTodayAtEndOfDay();
    void savesSeparateDateHourAndMinuteControls();
    void canCreateTaskWithoutDueTime();
    void lookupAndImportanceChoicesKeepTheirColors();
    void savesTaskDefaultTimerPreset();
};

void TaskDialogTests::newTaskDefaultsToTodayAtEndOfDay()
{
    TaskDialog dialog({}, {}, {});

    auto *enabled = dialog.findChild<QCheckBox *>(QStringLiteral("dueEnabled"));
    auto *dateEdit = dialog.findChild<QDateTimeEdit *>(QStringLiteral("dueDateEdit"));
    auto *hour = dialog.findChild<QSpinBox *>(QStringLiteral("dueHourSpin"));
    auto *minute = dialog.findChild<QSpinBox *>(QStringLiteral("dueMinuteSpin"));

    QVERIFY(enabled != nullptr);
    QVERIFY(dateEdit != nullptr);
    QVERIFY(hour != nullptr);
    QVERIFY(minute != nullptr);
    QVERIFY(enabled->isChecked());
    QCOMPARE(dateEdit->date(), QDate::currentDate());
    QCOMPARE(hour->value(), 23);
    QCOMPARE(minute->value(), 59);

    const Task task = dialog.task();
    QVERIFY(task.dueAt.isValid());
    QCOMPARE(task.dueAt.date(), QDate::currentDate());
    QCOMPARE(task.dueAt.time(), QTime(23, 59));
}

void TaskDialogTests::savesSeparateDateHourAndMinuteControls()
{
    TaskDialog dialog({}, {}, {});
    auto *dateEdit = dialog.findChild<QDateTimeEdit *>(QStringLiteral("dueDateEdit"));
    auto *hour = dialog.findChild<QSpinBox *>(QStringLiteral("dueHourSpin"));
    auto *minute = dialog.findChild<QSpinBox *>(QStringLiteral("dueMinuteSpin"));
    QVERIFY(dateEdit != nullptr);
    QVERIFY(hour != nullptr);
    QVERIFY(minute != nullptr);

    const QDate selectedDate = QDate::currentDate().addDays(3);
    dateEdit->setDate(selectedDate);
    hour->setValue(14);
    minute->setValue(35);

    const Task task = dialog.task();
    QCOMPARE(task.dueAt, QDateTime(selectedDate, QTime(14, 35)));
}

void TaskDialogTests::canCreateTaskWithoutDueTime()
{
    TaskDialog dialog({}, {}, {});
    auto *enabled = dialog.findChild<QCheckBox *>(QStringLiteral("dueEnabled"));
    auto *dateEdit = dialog.findChild<QDateTimeEdit *>(QStringLiteral("dueDateEdit"));
    auto *hour = dialog.findChild<QSpinBox *>(QStringLiteral("dueHourSpin"));
    auto *minute = dialog.findChild<QSpinBox *>(QStringLiteral("dueMinuteSpin"));
    QVERIFY(enabled != nullptr);
    QVERIFY(dateEdit != nullptr);
    QVERIFY(hour != nullptr);
    QVERIFY(minute != nullptr);

    enabled->setChecked(false);

    QVERIFY(!dateEdit->isEnabled());
    QVERIFY(!hour->isEnabled());
    QVERIFY(!minute->isEnabled());
    QVERIFY(!dialog.task().dueAt.isValid());
}

void TaskDialogTests::lookupAndImportanceChoicesKeepTheirColors()
{
    const LookupItem project{7, QStringLiteral("蓝色项目"),
                             QStringLiteral("#175CD3")};
    const LookupItem category{8, QStringLiteral("紫色分类"),
                              QStringLiteral("#6941C6")};
    TaskDialog dialog({project}, {category}, {});

    auto *projectCombo = dialog.findChild<QComboBox *>(
        QStringLiteral("taskProjectCombo"));
    auto *categoryCombo = dialog.findChild<QComboBox *>(
        QStringLiteral("taskCategoryCombo"));
    auto *importanceCombo = dialog.findChild<QComboBox *>(
        QStringLiteral("taskImportanceCombo"));
    QVERIFY(projectCombo != nullptr);
    QVERIFY(categoryCombo != nullptr);
    QVERIFY(importanceCombo != nullptr);
    QCOMPARE(importanceCombo->count(), 5);

    for (int index = 0; index < importanceCombo->count(); ++index) {
        const int level = index + 1;
        QVERIFY(importanceCombo->itemText(index).contains(
            QString(level, QChar(0x2605))));
        QCOMPARE(importanceCombo->itemData(index, Qt::ForegroundRole)
                     .value<QColor>(),
                 PriorityColors::importance(level));
    }

    projectCombo->setCurrentIndex(projectCombo->findData(project.id));
    categoryCombo->setCurrentIndex(categoryCombo->findData(category.id));
    importanceCombo->setCurrentIndex(4);
    QCOMPARE(projectCombo->palette().color(QPalette::Text),
             QColor(project.color));
    QCOMPARE(categoryCombo->palette().color(QPalette::Text),
             QColor(category.color));
    QCOMPARE(importanceCombo->palette().color(QPalette::Text),
             PriorityColors::importance(5));
}

void TaskDialogTests::savesTaskDefaultTimerPreset()
{
    TimerPreset defaultPreset;
    defaultPreset.id = 3;
    defaultPreset.name = QStringLiteral("经典番茄钟");
    defaultPreset.isDefault = true;
    TimerPreset deepWork;
    deepWork.id = 7;
    deepWork.name = QStringLiteral("深度工作");
    deepWork.focusMinutes = 50;
    deepWork.shortBreakMinutes = 10;

    TaskDialog dialog({}, {}, {defaultPreset, deepWork});
    auto *combo = dialog.findChild<QComboBox *>(
        QStringLiteral("taskTimerPresetCombo"));
    QVERIFY(combo != nullptr);
    QCOMPARE(combo->itemData(0).toInt(), -1);
    QVERIFY(combo->itemText(0).contains(QStringLiteral("跟随系统默认")));
    combo->setCurrentIndex(combo->findData(deepWork.id));
    QCOMPARE(dialog.task().timerPresetId, deepWork.id);
}

QTEST_MAIN(TaskDialogTests)

#include "tst_TaskDialog.moc"
