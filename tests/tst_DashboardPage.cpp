#include "data/DatabaseManager.h"
#include "repositories/AnalyticsRepository.h"
#include "repositories/FocusRepository.h"
#include "repositories/ProjectRepository.h"
#include "repositories/SettingsRepository.h"
#include "repositories/TaskRepository.h"
#include "views/DashboardPage.h"
#include "views/ProjectPage.h"
#include "views/StatisticsPage.h"
#include "views/TaskPage.h"
#include "widgets/PriorityColors.h"
#include "widgets/StatusColors.h"

#include <QBarSet>
#include <QBarSeries>
#include <QChartView>
#include <QCoreApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QHeaderView>
#include <QListWidget>
#include <QSignalSpy>
#include <QScrollArea>
#include <QSet>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QTest>
#include <QToolTip>
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
    void todayFocusMetricShowsSeconds();
    void recentFocusTableShowsTenRowsWithoutNestedScrolling();
    void recommendationSupportsFocusShortcutAndBlankDeselection();
    void recommendationFiltersByProjectAndCategory();
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
    auto *projectFilter = page.findChild<QComboBox *>(
        QStringLiteral("recommendationProjectFilter"));

    QVERIFY(title != nullptr);
    QVERIFY(description != nullptr);
    QVERIFY(emptyState != nullptr);
    QVERIFY(content != nullptr);
    QVERIFY(list != nullptr);
    QVERIFY(projectFilter != nullptr);
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
    const int filterTop = projectFilter->mapTo(card, QPoint(0, 0)).y();
    const int filterBottom = filterTop + projectFilter->height();
    const int emptyStateTop = emptyState->mapTo(card, QPoint(0, 0)).y();

    QVERIFY(titleTop <= 30);
    QVERIFY(descriptionTop >= titleBottom);
    QVERIFY(descriptionTop - titleBottom <= 16);
    QVERIFY(filterTop >= descriptionBottom);
    QVERIFY(filterTop - descriptionBottom <= 20);
    QVERIFY(emptyStateTop >= filterBottom);
    QVERIFY(emptyStateTop - filterBottom <= 20);
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
            QCOMPARE(QColor(categoryFocus.color),
                     QColor(QStringLiteral("#4F6EF7")));
            foundCategory = true;
        }
    }
    QVERIFY(foundCategory);

    const auto projectFocus = analytics.focusByProject();
    QVERIFY(!projectFocus.isEmpty());
    QCOMPARE(projectFocus.constFirst().name, QStringLiteral("无项目"));
    QCOMPARE(projectFocus.constFirst().focusSeconds, 59);
    QCOMPARE(QColor(projectFocus.constFirst().color),
             QColor(QStringLiteral("#667085")));

    const auto recentSessions = analytics.recentFocusSessions();
    QVERIFY(!recentSessions.isEmpty());
    QCOMPARE(recentSessions.constFirst().taskName,
             QStringLiteral("一分钟测试任务"));
    QCOMPARE(recentSessions.constFirst().projectName,
             QStringLiteral("无项目"));
    QCOMPARE(recentSessions.constFirst().categoryName, categoryName);
    QCOMPARE(QColor(recentSessions.constFirst().categoryColor),
             QColor(QStringLiteral("#4F6EF7")));
    QCOMPARE(recentSessions.constFirst().focusSeconds, 59);
    QVERIFY(!recentSessions.constFirst().completed);

    const QColor updatedCategoryColor(QStringLiteral("#B42318"));
    QSqlQuery updateCategory(database);
    updateCategory.prepare(QStringLiteral(
        "UPDATE categories SET color = :color WHERE id = :id"));
    updateCategory.bindValue(QStringLiteral(":color"),
                             updatedCategoryColor.name());
    updateCategory.bindValue(QStringLiteral(":id"), categoryId);
    QVERIFY(updateCategory.exec());

    StatisticsPage refreshedStatistics;
    auto *recentTable = refreshedStatistics.findChild<QTableWidget *>(
        QStringLiteral("recentFocusSessions"));
    QVERIFY(recentTable != nullptr);
    bool foundUpdatedColor = false;
    for (int row = 0; row < recentTable->rowCount(); ++row) {
        if (recentTable->item(row, 4)->text() == categoryName) {
            QCOMPARE(recentTable->item(row, 4)->foreground().color(),
                     updatedCategoryColor);
            QCOMPARE(recentTable->item(row, 6)->text(),
                     QStringLiteral("已终止"));
            QCOMPARE(recentTable->item(row, 6)->foreground().color(),
                     StatusColors::focusResult(false));
            foundUpdatedColor = true;
            break;
        }
    }
    QVERIFY(foundUpdatedColor);
}

void DashboardPageTests::todayFocusMetricShowsSeconds()
{
    DashboardPage page;
    QLabel *focusValue = nullptr;
    const auto labels = page.findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label->property("dashboardMetric").toString()
            == QStringLiteral("focusToday")) {
            focusValue = label;
            break;
        }
    }
    QVERIFY(focusValue != nullptr);
    QCOMPARE(focusValue->text(), QStringLiteral("59秒"));
}

void DashboardPageTests::recentFocusTableShowsTenRowsWithoutNestedScrolling()
{
    QSqlQuery insert(DatabaseManager::instance().database());
    insert.prepare(QStringLiteral(R"(
        INSERT INTO focus_sessions(
            session_type, status, start_time, end_time,
            planned_seconds, actual_seconds, interruption_reason
        ) VALUES('focus', 'completed', :started_at, :started_at, 1, 1, '')
    )"));
    for (int index = 0; index < 12; ++index) {
        insert.bindValue(
            QStringLiteral(":started_at"),
            QDateTime::currentDateTime().addSecs(-100 - index).toString(Qt::ISODate));
        QVERIFY(insert.exec());
    }

    StatisticsPage page;
    page.resize(1200, 800);
    page.show();
    QCoreApplication::processEvents();

    auto *tabs = page.findChild<QTabWidget *>(QStringLiteral("statisticsTabs"));
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(0), QStringLiteral("图表分析"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("最近记录"));
    QCOMPARE(tabs->tabBar()->objectName(),
             QStringLiteral("statisticsModeSwitch"));
    QCOMPARE(tabs->currentIndex(), 0);
    QVERIFY(tabs->widget(0)->isVisible());
    QVERIFY(!tabs->widget(1)->isVisible());
    tabs->setCurrentIndex(1);
    QCoreApplication::processEvents();
    QVERIFY(!tabs->widget(0)->isVisible());
    QVERIFY(tabs->widget(1)->isVisible());

    auto *table = page.findChild<QTableWidget *>(
        QStringLiteral("recentFocusSessions"));
    auto *hint = page.findChild<QLabel *>(QStringLiteral("recentFocusHint"));
    QVERIFY(table != nullptr);
    QVERIFY(hint != nullptr);
    QCOMPARE(table->columnCount(), 7);
    QCOMPARE(table->rowCount(), 10);
    QCOMPARE(table->horizontalHeaderItem(0)->text(), QStringLiteral("编号"));
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("1"));
    QCOMPARE(table->item(9, 0)->text(), QStringLiteral("10"));
    bool foundCompletedResult = false;
    bool foundInterruptedResult = false;
    for (int row = 0; row < table->rowCount(); ++row) {
        const QString result = table->item(row, 6)->text();
        if (result == QStringLiteral("已完成")) {
            QCOMPARE(table->item(row, 6)->foreground().color(),
                     StatusColors::focusResult(true));
            foundCompletedResult = true;
        } else if (result == QStringLiteral("已终止")) {
            QCOMPARE(table->item(row, 6)->foreground().color(),
                     StatusColors::focusResult(false));
            foundInterruptedResult = true;
        }
    }
    QVERIFY(foundCompletedResult);
    QVERIFY(foundInterruptedResult);
    QCOMPARE(table->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(table->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(table->verticalHeader()->sectionResizeMode(0),
             QHeaderView::Stretch);
    QVERIFY(table->isSortingEnabled());
    QVERIFY(table->horizontalHeader()->sectionsClickable());
    QVERIFY(hint->text().contains(QStringLiteral("最近 10 条")));
    QVERIFY(page.findChild<QScrollArea *>() == nullptr);

    auto *dailyChartView = page.findChild<QChartView *>(
        QStringLiteral("dailyFocusChartView"));
    QVERIFY(dailyChartView != nullptr);
    QVERIFY(!dailyChartView->isVisible());
    tabs->setCurrentIndex(0);
    QCoreApplication::processEvents();
    QVERIFY(dailyChartView->isVisible());
    QVERIFY(!tabs->widget(1)->isVisible());
    QVERIFY(!dailyChartView->chart()->series().isEmpty());
    auto *dailySeries = qobject_cast<QBarSeries *>(
        dailyChartView->chart()->series().constFirst());
    QVERIFY(dailySeries != nullptr);
    QVERIFY(!dailySeries->barSets().isEmpty());
    auto *dailyFocusSet = dailySeries->barSets().constFirst();
    QVERIFY(dailyFocusSet != nullptr);
    QVERIFY(QMetaObject::invokeMethod(dailyFocusSet, "hovered",
                                      Q_ARG(bool, true), Q_ARG(int, 6)));
    QCoreApplication::processEvents();
    QVERIFY(QToolTip::text().contains(QStringLiteral("秒")));
    QVERIFY(dailyChartView->property("focusTooltipAnchor").toPoint()
                != QPoint());
    QCOMPARE(dailyChartView->property("focusTooltipIndex").toInt(), 6);
    QVERIFY(QMetaObject::invokeMethod(dailyFocusSet, "hovered",
                                      Q_ARG(bool, false), Q_ARG(int, 6)));

    tabs->setCurrentIndex(1);
    QCoreApplication::processEvents();

    const QRect lastRow = table->visualItemRect(table->item(9, 0));
    QVERIFY(lastRow.isValid());
    QVERIFY(table->viewport()->rect().contains(lastRow.bottomLeft()));

    table->selectRow(0);
    QVERIFY(!table->selectedItems().isEmpty());
    QFrame *recentCard = nullptr;
    for (QFrame *frame : page.findChildren<QFrame *>()) {
        if (frame->property("statisticsSection").toString()
            == QStringLiteral("recentFocusCard")) {
            recentCard = frame;
            break;
        }
    }
    QVERIFY(recentCard != nullptr);
    QTest::mouseClick(recentCard, Qt::LeftButton, Qt::NoModifier,
                      QPoint(5, 5));
    QVERIFY(table->selectedItems().isEmpty());
    QVERIFY(!table->currentIndex().isValid());
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
    auto *scoreLine = list->itemWidget(item)->findChild<QLabel *>(
        QStringLiteral("recommendationScoreLine"));
    QVERIFY(scoreLine != nullptr);
    QVERIFY(item->text().isEmpty());
    QVERIFY(item->data(Qt::AccessibleTextRole).toString().contains(
        QStringLiteral("推荐分")));
    QCOMPARE(list->itemWidget(item)->findChildren<QLabel *>().size(), 2);
    QVERIFY(scoreLine->text().contains(QStringLiteral("推荐分")));
    QVERIFY(scoreLine->text().contains(QStringLiteral("color:")));
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

void DashboardPageTests::recommendationFiltersByProjectAndCategory()
{
    const QString suffix =
        QUuid::createUuid().toString(QUuid::WithoutBraces);
    Project project;
    project.name = QStringLiteral("筛选项目-%1").arg(suffix);
    project.description = QStringLiteral("");
    project.color = QStringLiteral("#4F6EF7");
    ProjectRepository projectRepository;
    QString error;
    QVERIFY2(projectRepository.saveProject(project, &error), qPrintable(error));

    LookupItem category;
    category.name = QStringLiteral("筛选分类-%1").arg(suffix);
    category.color = QStringLiteral("#C335B4");
    QVERIFY2(projectRepository.saveCategory(category, &error), qPrintable(error));

    Task task;
    task.title = QStringLiteral("筛选建议任务-%1").arg(suffix);
    task.description = QStringLiteral("");
    task.projectId = project.id;
    task.categoryId = category.id;
    task.importance = 5;
    task.dueAt = QDateTime::currentDateTime().addDays(1);
    QVERIFY2(TaskRepository().save(task, &error), qPrintable(error));

    DashboardPage page;
    auto *projectFilter = page.findChild<QComboBox *>(
        QStringLiteral("recommendationProjectFilter"));
    auto *categoryFilter = page.findChild<QComboBox *>(
        QStringLiteral("recommendationCategoryFilter"));
    auto *list = page.findChild<QListWidget *>(
        QStringLiteral("recommendationList"));
    QVERIFY(projectFilter != nullptr);
    QVERIFY(categoryFilter != nullptr);
    QVERIFY(list != nullptr);

    const int projectIndex = projectFilter->findData(project.id);
    const int categoryIndex = categoryFilter->findData(category.id);
    QVERIFY(projectIndex >= 0);
    QVERIFY(categoryIndex >= 0);
    projectFilter->setCurrentIndex(projectIndex);
    categoryFilter->setCurrentIndex(categoryIndex);
    QCoreApplication::processEvents();

    QCOMPARE(list->count(), 1);
    QListWidgetItem *item = list->item(0);
    QCOMPARE(item->data(Qt::UserRole).toInt(), task.id);
    QCOMPARE(item->data(Qt::UserRole + 1).toInt(), project.id);
    QCOMPARE(item->data(Qt::UserRole + 2).toInt(), category.id);
    QCOMPARE(projectFilter->palette().color(QPalette::Text),
             QColor(project.color));
    QCOMPARE(categoryFilter->palette().color(QPalette::Text),
             QColor(category.color));
    auto *scoreLine = list->itemWidget(item)->findChild<QLabel *>(
        QStringLiteral("recommendationScoreLine"));
    QVERIFY(scoreLine != nullptr);
    QVERIFY(scoreLine->text().toLower().contains(project.color.toLower()));
    QVERIFY(scoreLine->text().toLower().contains(category.color.toLower()));
    QVERIFY(scoreLine->text().contains(
        task.dueAt.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
    const QString accessibleText =
        item->data(Qt::AccessibleTextRole).toString();
    QVERIFY(accessibleText.contains(QStringLiteral("项目：%1").arg(project.name)));
    QVERIFY(accessibleText.contains(QStringLiteral("分类：%1").arg(category.name)));
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
    highImportance.dueAt = QDateTime::currentDateTime().addDays(-1);
    Task completedTask;
    completedTask.title = QStringLiteral("排序测试-已完成");
    completedTask.description = QStringLiteral("");
    completedTask.importance = 3;
    completedTask.estimatedMinutes = 25;
    completedTask.status = QStringLiteral("completed");
    QString error;
    TaskRepository taskRepository;
    QVERIFY2(taskRepository.save(lowImportance, &error), qPrintable(error));
    QVERIFY2(taskRepository.save(highImportance, &error), qPrintable(error));
    QVERIFY2(taskRepository.save(completedTask, &error), qPrintable(error));

    TaskPage taskPage;
    taskPage.resize(1800, 800);
    taskPage.show();
    QCoreApplication::processEvents();
    auto *taskTable =
        taskPage.findChild<QTableWidget *>(QStringLiteral("taskTable"));
    QVERIFY(taskTable != nullptr);
    QCOMPARE(taskTable->horizontalHeader()->sortIndicatorSection(), 5);
    QCOMPARE(taskTable->horizontalHeader()->sortIndicatorOrder(),
             Qt::DescendingOrder);
    for (int row = 1; row < taskTable->rowCount(); ++row) {
        const double previous =
            taskTable->item(row - 1, 5)->data(Qt::UserRole + 1).toDouble();
        const double current =
            taskTable->item(row, 5)->data(Qt::UserRole + 1).toDouble();
        QVERIFY(previous >= current);
    }

    QCOMPARE(taskTable->horizontalHeaderItem(1)->text(),
             QStringLiteral("详细描述"));
    bool foundCenteredEmptyProject = false;
    for (int row = 0; row < taskTable->rowCount(); ++row) {
        for (int column = 0; column < taskTable->columnCount(); ++column) {
            QCOMPARE(taskTable->item(row, column)->textAlignment(),
                     static_cast<int>(Qt::AlignCenter));
        }
        if (taskTable->item(row, 2)->text() == QStringLiteral("—")) {
            QCOMPARE(taskTable->item(row, 2)->textAlignment(),
                     static_cast<int>(Qt::AlignCenter));
            foundCenteredEmptyProject = true;
        }
    }
    QVERIFY(foundCenteredEmptyProject);

    QSet<QString> importanceColors;
    for (int level = 1; level <= 5; ++level) {
        importanceColors.insert(PriorityColors::importance(level).name());
    }
    QCOMPARE(importanceColors.size(), 5);
    QSet<QString> recommendationColors;
    for (const int score : {20, 50, 80, 110, 140}) {
        recommendationColors.insert(
            PriorityColors::recommendation(score).name());
    }
    QCOMPARE(recommendationColors.size(), 5);

    QColor lowImportanceColor;
    QColor highImportanceColor;
    QColor lowScoreColor;
    QColor highScoreColor;
    QColor overdueColor;
    QColor pendingStatusColor;
    QColor completedStatusColor;
    QString overdueText;
    for (int row = 0; row < taskTable->rowCount(); ++row) {
        const QString title = taskTable->item(row, 0)->text();
        if (title == lowImportance.title) {
            lowImportanceColor = taskTable->item(row, 5)->foreground().color();
            lowScoreColor = taskTable->item(row, 7)->foreground().color();
            pendingStatusColor = taskTable->item(row, 8)->foreground().color();
        } else if (title == highImportance.title) {
            highImportanceColor = taskTable->item(row, 5)->foreground().color();
            highScoreColor = taskTable->item(row, 7)->foreground().color();
            overdueColor = taskTable->item(row, 4)->foreground().color();
            overdueText = taskTable->item(row, 4)->text();
        } else if (title == completedTask.title) {
            QCOMPARE(taskTable->item(row, 8)->text(),
                     QStringLiteral("已完成"));
            completedStatusColor =
                taskTable->item(row, 8)->foreground().color();
        }
    }
    QVERIFY(lowImportanceColor.isValid());
    QVERIFY(highImportanceColor.isValid());
    QCOMPARE(lowImportanceColor, PriorityColors::importance(1));
    QCOMPARE(highImportanceColor, PriorityColors::importance(5));
    QVERIFY(lowScoreColor != highScoreColor);
    QCOMPARE(overdueColor, QColor(QStringLiteral("#B42318")));
    QCOMPARE(pendingStatusColor, StatusColors::taskStatus(
                                       QStringLiteral("pending")));
    QCOMPARE(completedStatusColor, StatusColors::taskStatus(
                                         QStringLiteral("completed")));
    QVERIFY(pendingStatusColor != completedStatusColor);
    QVERIFY(overdueText.contains(QStringLiteral("已逾期")));
    QVERIFY(overdueText.contains(QString::number(QDate::currentDate().year())));
    QCOMPARE(taskTable->horizontalHeader()->sectionResizeMode(0),
             QHeaderView::Stretch);
    QCOMPARE(taskTable->horizontalHeader()->sectionResizeMode(1),
             QHeaderView::Stretch);
    int occupiedWidth = 0;
    for (int column = 0; column < taskTable->columnCount(); ++column) {
        occupiedWidth += taskTable->columnWidth(column);
    }
    QVERIFY(occupiedWidth >= taskTable->viewport()->width() - 2);

    taskTable->sortItems(6, Qt::AscendingOrder);
    for (int row = 1; row < taskTable->rowCount(); ++row) {
        const double previous =
            taskTable->item(row - 1, 6)->data(Qt::UserRole + 1).toDouble();
        const double current =
            taskTable->item(row, 6)->data(Qt::UserRole + 1).toDouble();
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
    LookupItem coloredCategory;
    coloredCategory.name = QStringLiteral("排序测试-彩色分类");
    coloredCategory.color = QStringLiteral("#175CD3");
    QVERIFY2(projectRepository.saveCategory(coloredCategory, &error),
             qPrintable(error));

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
    for (int row = 0; row < projectTable->rowCount(); ++row) {
        for (int column = 0; column < projectTable->columnCount(); ++column) {
            QCOMPARE(projectTable->item(row, column)->textAlignment(),
                     static_cast<int>(Qt::AlignCenter));
        }
        const QString projectName = projectTable->item(row, 0)->text();
        if (projectName == activeProject.name) {
            QCOMPARE(projectTable->item(row, 0)->foreground().color(),
                     QColor(activeProject.color));
            QCOMPARE(projectTable->item(row, 3)->text(),
                     QStringLiteral("进行中"));
            QCOMPARE(projectTable->item(row, 3)->foreground().color(),
                     StatusColors::projectStatus(false));
        } else if (projectName == archivedProject.name) {
            QCOMPARE(projectTable->item(row, 0)->foreground().color(),
                     QColor(archivedProject.color));
            QCOMPARE(projectTable->item(row, 3)->text(),
                     QStringLiteral("已归档"));
            QCOMPARE(projectTable->item(row, 3)->foreground().color(),
                     StatusColors::projectStatus(true));
        }
    }
    for (int row = 0; row < categoryTable->rowCount(); ++row) {
        for (int column = 0; column < categoryTable->columnCount(); ++column) {
            QCOMPARE(categoryTable->item(row, column)->textAlignment(),
                     static_cast<int>(Qt::AlignCenter));
        }
        if (categoryTable->item(row, 0)->text() == coloredCategory.name) {
            QCOMPARE(categoryTable->item(row, 0)->foreground().color(),
                     QColor(coloredCategory.color));
        }
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
