#include "views/TaskDialog.h"

#include <QCheckBox>
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
};

void TaskDialogTests::newTaskDefaultsToTodayAtEndOfDay()
{
    TaskDialog dialog({}, {});

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
    TaskDialog dialog({}, {});
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
    TaskDialog dialog({}, {});
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

QTEST_MAIN(TaskDialogTests)

#include "tst_TaskDialog.moc"
