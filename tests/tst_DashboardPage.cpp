#include "data/DatabaseManager.h"
#include "repositories/AnalyticsRepository.h"
#include "repositories/FocusRepository.h"
#include "repositories/ProjectRepository.h"
#include "repositories/SettingsRepository.h"
#include "repositories/TaskRepository.h"
#include "views/DashboardPage.h"
#include "views/ProjectPage.h"
#include "views/TaskPage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QHeaderView>
#include <QListWidget>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTest>
#include <QUuid>

namespace {
QLabel *findLabelByText(const QWidget &parent, const QString &text)
{
    const auto labels = parent.findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label->text() == text) {
            return label;
        }
    }
    return nullptr;
}
}

class DashboardPageTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void emptyStateKeepsContentTopAligned();
    void interruptedFocusIsIncludedInStatistics();
    void recommendationSupportsFocusShortcutAndBlankDeselection();
    void taskDeletionIsPermanentAndPreservesFocusHistory();
    void completedFocusAllowsEmptyInterruptionReason();
    void closeToTrayReminderPreferenceRoundTrips();
    void projectDeletionKeepsTasksWithoutProject();
    void tablesExposeSafePredictableSorting();
    void cleanupTestCase();

private:
    QString dataDirectory_;
};

void DashboardPageTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlowTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("Dashboard-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));

    QVERIFY2(DatabaseManager::instance().initialize(),
             qPrintable(DatabaseManager::instance().lastError()));
    dataDirectory_ =
        QFileInfo(DatabaseManager::instance().databasePath()).absolutePath();
}

void DashboardPageTests::emptyStateKeepsContentTopAligned()
{
    DashboardPage page;
    page.resize(1200, 700);
    page.show();
    QCoreApplication::processEvents();

    auto *title = findLabelByText(page, QStringLiteral("优先建议"));
    auto *description = findLabelByText(
        page, QStringLiteral("综合重要程度、截止时间、逾期情况和预计耗时排序。"));
    auto *emptyState =
        page.findChild<QLabel *>(QStringLiteral("emptyStateLabel"));
    auto *content =
        page.findChild<QStackedWidget *>(QStringLiteral("recommendationContent"));
    auto *list =
        page.findChild<QListWidget *>(QStringLiteral("recommendationList"));

    QVERIFY(title != nullptr);
    QVERIFY(description != nullptr);
    QVERIFY(emptyState != nullptr);
    QVERIFY(content != nullptr);
    QVERIFY(list != nullptr);
    QCOMPARE(content->currentIndex(), 1);
    QVERIFY(emptyState->isVisible());
    QVERIFY(!list->isVisible());
    QVERIFY(emptyState->testAttribute(Qt::WA_TransparentForMouseEvents));

    QWidget *card = title->parentWidget();
    QVERIFY(card != nullptr);
    const int titleTop = title->mapTo(card, QPoint(0, 0)).y();
    const int titleBottom = titleTop + title->height();
    const int descriptionTop = description->mapTo(card, QPoint(0, 0)).y();
    const int descriptionBottom = descriptionTop + description->height();
    const int emptyStateTop = emptyState->mapTo(card, QPoint(0, 0)).y();

    QVERIFY(titleTop <= 30);
    QVERIFY(descriptionTop >= titleBottom);
    QVERIFY(descriptionTop - titleBottom <= 16);
    QVERIFY(emptyStateTop >= descriptionBottom);
    QVERIFY(emptyStateTop - descriptionBottom <= 20);
    QVERIFY(emptyStateTop < card->height() / 4);
}

void DashboardPageTests::interruptedFocusIsIncludedInStatistics()
{
    auto database = DatabaseManager::instance().database();
    const QString categoryName = QStringLiteral("测试分类-%1")
                                     .arg(QUuid::createUuid().toString(
                                         QUuid::WithoutBraces));

    QSqlQuery category(database);
    category.prepare(QStringLiteral(
        "INSERT INTO categories(name, color) VALUES(:name, '#4f6ef7')"));
    category.bindValue(QStringLiteral(":name"), categoryName);
    QVERIFY(category.exec());
    const int categoryId = category.lastInsertId().toInt();

    QSqlQuery task(database);
    task.prepare(QStringLiteral(R"(
        INSERT INTO tasks(title, category_id, estimated_minutes, status)
        VALUES('一分钟测试任务', :category_id, 1, 'pending')
    )"));
    task.bindValue(QStringLiteral(":category_id"), categoryId);
    QVERIFY(task.exec());
    const int taskId = task.lastInsertId().toInt();

    const QDateTime startedAt = QDateTime::currentDateTime().addSecs(-59);
    QSqlQuery session(database);
    session.prepare(QStringLiteral(R"(
        INSERT INTO focus_sessions(
            task_id, session_type, status, start_time, end_time,
            planned_seconds, actual_seconds, interruption_reason
        ) VALUES(
            :task_id, 'focus', 'interrupted', :start_time, :end_time,
            60, 59, '用户提前结束'
        )
    )"));
    session.bindValue(QStringLiteral(":task_id"), taskId);
    session.bindValue(QStringLiteral(":start_time"), startedAt.toString(Qt::ISODate));
    session.bindValue(QStringLiteral(":end_time"),
                      QDateTime::currentDateTime().toString(Qt::ISODate));
    QVERIFY(session.exec());

    session.prepare(QStringLiteral(R"(
        INSERT INTO focus_sessions(
            task_id, session_type, status, start_time, end_time,
            planned_seconds, actual_seconds, interruption_reason
        ) VALUES(
            :task_id, 'short_break', 'interrupted', :start_time, :end_time,
            300, 120, '用户提前结束'
        )
    )"));
    session.bindValue(QStringLiteral(":task_id"), taskId);
    session.bindValue(QStringLiteral(":start_time"), startedAt.toString(Qt::ISODate));
    session.bindValue(QStringLiteral(":end_time"),
                      QDateTime::currentDateTime().toString(Qt::ISODate));
    QVERIFY(session.exec());

    const AnalyticsRepository analytics;
    const DashboardMetrics metrics = analytics.dashboardMetrics();
    QCOMPARE(metrics.focusSecondsToday, 59);
    QCOMPARE(metrics.focusSecondsLastSevenDays, 59);

    bool foundToday = false;
    const QString todayLabel =
        QDate::currentDate().toString(QStringLiteral("MM-dd"));
    for (const DailyProductivity &day : analytics.lastSevenDays()) {
        if (day.label == todayLabel) {
            QCOMPARE(day.focusSeconds, 59);
            foundToday = true;
        }
    }
    QVERIFY(foundToday);

    bool foundCategory = false;
    for (const CategoryFocus &categoryFocus : analytics.focusByCategory()) {
        if (categoryFocus.name == categoryName) {
            QCOMPARE(categoryFocus.focusSeconds, 59);
            foundCategory = true;
        }
    }
    QVERIFY(foundCategory);
}

void DashboardPageTests::recommendationSupportsFocusShortcutAndBlankDeselection()
{
    DashboardPage page;
    page.resize(1200, 700);
    page.show();
    QCoreApplication::processEvents();

    auto *list =
        page.findChild<QListWidget *>(QStringLiteral("recommendationList"));
    QVERIFY(list != nullptr);
    QVERIFY(list->isVisible());
    QVERIFY(list->count() > 0);

    QListWidgetItem *item = list->item(0);
    const int taskId = item->data(Qt::UserRole).toInt();
    QVERIFY(taskId > 0);
    QSignalSpy focusRequestSpy(&page, &DashboardPage::focusTaskRequested);

    const QRect itemRect = list->visualItemRect(item);
    QVERIFY(itemRect.isValid());
    list->setFocus();
    QTest::mouseClick(list->viewport(), Qt::LeftButton,
                      Qt::NoModifier, itemRect.center());
    QTest::mouseDClick(list->viewport(), Qt::LeftButton,
                       Qt::NoModifier, itemRect.center());
    QCoreApplication::processEvents();
    QCOMPARE(focusRequestSpy.count(), 1);
    QCOMPARE(focusRequestSpy.takeFirst().at(0).toInt(), taskId);

    list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
    QVERIFY(!list->selectedItems().isEmpty());
    const QPoint blankPoint(list->viewport()->width() / 2,
                            list->viewport()->height() - 8);
    QVERIFY(list->itemAt(blankPoint) == nullptr);
    QTest::mouseClick(list->viewport(), Qt::LeftButton,
                      Qt::NoModifier, blankPoint);
    QVERIFY(list->selectedItems().isEmpty());
    QVERIFY(list->currentItem() == nullptr);
}

void DashboardPageTests::taskDeletionIsPermanentAndPreservesFocusHistory()
{
    auto database = DatabaseManager::instance().database();
    QSqlQuery task(database);
    QVERIFY(task.exec(QStringLiteral(R"(
        INSERT INTO tasks(title, status)
        VALUES('永久删除测试任务', 'pending')
    )")));
    const int taskId = task.lastInsertId().toInt();

    QSqlQuery session(database);
    session.prepare(QStringLiteral(R"(
        INSERT INTO focus_sessions(
            task_id, session_type, status, start_time, end_time,
            planned_seconds, actual_seconds
        ) VALUES(
            :task_id, 'focus', 'completed', :now, :now, 60, 60
        )
    )"));
    session.bindValue(QStringLiteral(":task_id"), taskId);
    session.bindValue(QStringLiteral(":now"),
                      QDateTime::currentDateTime().toString(Qt::ISODate));
    QVERIFY(session.exec());
    const int sessionId = session.lastInsertId().toInt();

    QString error;
    QVERIFY2(TaskRepository().deleteTask(taskId, &error), qPrintable(error));

    QSqlQuery deletedTask(database);
    deletedTask.prepare(QStringLiteral("SELECT COUNT(*) FROM tasks WHERE id = :id"));
    deletedTask.bindValue(QStringLiteral(":id"), taskId);
    QVERIFY(deletedTask.exec());
    QVERIFY(deletedTask.next());
    QCOMPARE(deletedTask.value(0).toInt(), 0);

    QSqlQuery preservedSession(database);
    preservedSession.prepare(QStringLiteral(
        "SELECT task_id FROM focus_sessions WHERE id = :id"));
    preservedSession.bindValue(QStringLiteral(":id"), sessionId);
    QVERIFY(preservedSession.exec());
    QVERIFY(preservedSession.next());
    QVERIFY(preservedSession.value(0).isNull());
}

void DashboardPageTests::completedFocusAllowsEmptyInterruptionReason()
{
    const QDateTime endedAt = QDateTime::currentDateTime();
    const QDateTime startedAt = endedAt.addSecs(-60);
    QString error;
    QVERIFY2(FocusRepository().recordSession(
                 -1,
                 FocusTimer::Phase::Focus,
                 true,
                 startedAt,
                 endedAt,
                 60,
                 60,
                 QString(),
                 &error),
             qPrintable(error));

    QSqlQuery session(DatabaseManager::instance().database());
    QVERIFY(session.exec(QStringLiteral(R"(
        SELECT status, interruption_reason
        FROM focus_sessions
        ORDER BY id DESC
        LIMIT 1
    )")));
    QVERIFY(session.next());
    QCOMPARE(session.value(0).toString(), QStringLiteral("completed"));
    QVERIFY(!session.value(1).isNull());
    QCOMPARE(session.value(1).toString(), QStringLiteral(""));
}

void DashboardPageTests::closeToTrayReminderPreferenceRoundTrips()
{
    SettingsRepository repository;
    TimerSettings settings = repository.loadTimerSettings();
    QVERIFY(!settings.suppressCloseToTrayReminder);

    settings.suppressCloseToTrayReminder = true;
    QString error;
    QVERIFY2(repository.saveTimerSettings(settings, &error), qPrintable(error));
    QVERIFY(repository.loadTimerSettings().suppressCloseToTrayReminder);

    settings = repository.loadTimerSettings();
    settings.suppressCloseToTrayReminder = false;
    QVERIFY2(repository.saveTimerSettings(settings, &error), qPrintable(error));
    QVERIFY(!repository.loadTimerSettings().suppressCloseToTrayReminder);
}

void DashboardPageTests::projectDeletionKeepsTasksWithoutProject()
{
    auto database = DatabaseManager::instance().database();
    Project project;
    project.name = QStringLiteral("待删除项目-%1")
                       .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    project.description = QStringLiteral("验证项目删除行为");
    project.color = QStringLiteral("#4F6EF7");
    QString error;
    ProjectRepository repository;
    QVERIFY2(repository.saveProject(project, &error), qPrintable(error));
    QVERIFY(project.id > 0);

    QSqlQuery task(database);
    task.prepare(QStringLiteral(R"(
        INSERT INTO tasks(title, project_id, status)
        VALUES('项目删除后的保留任务', :project_id, 'pending')
    )"));
    task.bindValue(QStringLiteral(":project_id"), project.id);
    QVERIFY(task.exec());
    const int taskId = task.lastInsertId().toInt();

    QVERIFY2(repository.deleteProject(project.id, &error), qPrintable(error));

    QSqlQuery preservedTask(database);
    preservedTask.prepare(QStringLiteral(
        "SELECT project_id FROM tasks WHERE id = :id"));
    preservedTask.bindValue(QStringLiteral(":id"), taskId);
    QVERIFY(preservedTask.exec());
    QVERIFY(preservedTask.next());
    QVERIFY(preservedTask.value(0).isNull());
}

void DashboardPageTests::tablesExposeSafePredictableSorting()
{
    Task lowImportance;
    lowImportance.title = QStringLiteral("排序测试-低重要度");
    lowImportance.description = QStringLiteral("");
    lowImportance.importance = 1;
    lowImportance.estimatedMinutes = 90;
    Task highImportance;
    highImportance.title = QStringLiteral("排序测试-高重要度");
    highImportance.description = QStringLiteral("");
    highImportance.importance = 5;
    highImportance.estimatedMinutes = 5;
    QString error;
    TaskRepository taskRepository;
    QVERIFY2(taskRepository.save(lowImportance, &error), qPrintable(error));
    QVERIFY2(taskRepository.save(highImportance, &error), qPrintable(error));

    TaskPage taskPage;
    auto *taskTable =
        taskPage.findChild<QTableWidget *>(QStringLiteral("taskTable"));
    QVERIFY(taskTable != nullptr);
    QCOMPARE(taskTable->horizontalHeader()->sortIndicatorSection(), 4);
    QCOMPARE(taskTable->horizontalHeader()->sortIndicatorOrder(),
             Qt::DescendingOrder);
    for (int row = 1; row < taskTable->rowCount(); ++row) {
        const double previous =
            taskTable->item(row - 1, 4)->data(Qt::UserRole + 1).toDouble();
        const double current =
            taskTable->item(row, 4)->data(Qt::UserRole + 1).toDouble();
        QVERIFY(previous >= current);
    }

    taskTable->sortItems(5, Qt::AscendingOrder);
    for (int row = 1; row < taskTable->rowCount(); ++row) {
        const double previous =
            taskTable->item(row - 1, 5)->data(Qt::UserRole + 1).toDouble();
        const double current =
            taskTable->item(row, 5)->data(Qt::UserRole + 1).toDouble();
        QVERIFY(previous <= current);
    }

    Project activeProject;
    activeProject.name = QStringLiteral("排序测试-进行中");
    activeProject.description = QStringLiteral("");
    activeProject.color = QStringLiteral("#4F6EF7");
    Project archivedProject;
    archivedProject.name = QStringLiteral("排序测试-已归档");
    archivedProject.description = QStringLiteral("");
    archivedProject.color = QStringLiteral("#C335B4");
    archivedProject.archived = true;
    ProjectRepository projectRepository;
    QVERIFY2(projectRepository.saveProject(activeProject, &error), qPrintable(error));
    QVERIFY2(projectRepository.saveProject(archivedProject, &error), qPrintable(error));

    ProjectPage projectPage;
    auto *projectTable =
        projectPage.findChild<QTableWidget *>(QStringLiteral("projectTable"));
    auto *categoryTable =
        projectPage.findChild<QTableWidget *>(QStringLiteral("categoryTable"));
    QVERIFY(projectTable != nullptr);
    QVERIFY(categoryTable != nullptr);
    QCOMPARE(projectTable->horizontalHeader()->sortIndicatorSection(), 3);
    QCOMPARE(projectTable->horizontalHeader()->sortIndicatorOrder(),
             Qt::AscendingOrder);
    for (int row = 1; row < projectTable->rowCount(); ++row) {
        const int previous =
            projectTable->item(row - 1, 3)->data(Qt::UserRole + 1).toInt();
        const int current =
            projectTable->item(row, 3)->data(Qt::UserRole + 1).toInt();
        QVERIFY(previous <= current);
    }

    projectTable->sortItems(3, Qt::DescendingOrder);
    if (projectTable->rowCount() > 0) {
        QCOMPARE(projectTable->item(0, 3)->data(Qt::UserRole + 1).toInt(), 1);
        QVERIFY(projectTable->item(0, 2)->icon().isNull());
        QVERIFY(!projectTable->item(0, 2)
                     ->data(Qt::UserRole + 2)
                     .toString()
                     .isEmpty());
    }
    QVERIFY(projectTable->itemDelegateForColumn(2)
            != projectTable->itemDelegate());
    QVERIFY(categoryTable->itemDelegateForColumn(1)
            != categoryTable->itemDelegate());
}

void DashboardPageTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(DashboardPageTests)

#include "tst_DashboardPage.moc"
