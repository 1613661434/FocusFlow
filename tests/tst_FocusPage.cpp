#include "data/DatabaseManager.h"
#include "repositories/TaskRepository.h"
#include "views/FocusPage.h"
#include "widgets/PriorityColors.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QTest>
#include <QTimer>
#include <QUuid>

class FocusPageTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void primaryButtonTracksTimerState();
    void taskFiltersShowScoresAndSortRecommendations();
    void cleanupTestCase();

private:
    static QPushButton *buttonForRole(FocusPage &page, const QString &role);
    QString dataDirectory_;
};

void FocusPageTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlowTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("FocusPage-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QVERIFY2(DatabaseManager::instance().initialize(),
             qPrintable(DatabaseManager::instance().lastError()));
    dataDirectory_ = QFileInfo(
        DatabaseManager::instance().databasePath()).absolutePath();
}

QPushButton *FocusPageTests::buttonForRole(FocusPage &page,
                                           const QString &role)
{
    const auto buttons = page.findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->property("timerControl").toString() == role) {
            return button;
        }
    }
    return nullptr;
}

void FocusPageTests::primaryButtonTracksTimerState()
{
    FocusPage page;
    auto *primary = buttonForRole(page, QStringLiteral("primary"));
    auto *stop = buttonForRole(page, QStringLiteral("stop"));
    QVERIFY(primary);
    QVERIFY(stop);
    QCOMPARE(primary->text(), QStringLiteral("开始"));
    QVERIFY(primary->isEnabled());
    QVERIFY(!stop->isEnabled());
    QVERIFY(primary->minimumHeight() >= 44);
    QCOMPARE(primary->minimumSize(), stop->minimumSize());

    primary->click();
    QCOMPARE(primary->text(), QStringLiteral("暂停"));
    QVERIFY(stop->isEnabled());

    primary->click();
    QCOMPARE(primary->text(), QStringLiteral("继续"));
    QVERIFY(stop->isEnabled());

    primary->click();
    QCOMPARE(primary->text(), QStringLiteral("暂停"));

    QTimer::singleShot(0, [] {
        if (auto *dialog = qobject_cast<QMessageBox *>(
                QApplication::activeModalWidget())) {
            dialog->button(QMessageBox::Yes)->click();
        }
    });
    stop->click();
    QCOMPARE(primary->text(), QStringLiteral("开始"));
    QVERIFY(primary->isEnabled());
    QVERIFY(!stop->isEnabled());
}

void FocusPageTests::taskFiltersShowScoresAndSortRecommendations()
{
    const QString suffix =
        QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlQuery projectInsert(DatabaseManager::instance().database());
    projectInsert.prepare(QStringLiteral(
        "INSERT INTO projects(name, description, color) VALUES(:name, '', '#4F6EF7')"));
    projectInsert.bindValue(QStringLiteral(":name"),
                            QStringLiteral("专注筛选项目-%1").arg(suffix));
    QVERIFY(projectInsert.exec());
    const int projectId = projectInsert.lastInsertId().toInt();

    QSqlQuery categoryInsert(DatabaseManager::instance().database());
    categoryInsert.prepare(QStringLiteral(
        "INSERT INTO categories(name, color) VALUES(:name, '#C335B4')"));
    categoryInsert.bindValue(QStringLiteral(":name"),
                             QStringLiteral("专注筛选分类-%1").arg(suffix));
    QVERIFY(categoryInsert.exec());
    const int categoryId = categoryInsert.lastInsertId().toInt();

    Task highScore;
    highScore.title = QStringLiteral("高推荐任务-%1").arg(suffix);
    highScore.description = QStringLiteral("");
    highScore.projectId = projectId;
    highScore.categoryId = categoryId;
    highScore.importance = 5;
    highScore.estimatedMinutes = 30;
    Task lowScore = highScore;
    lowScore.id = -1;
    lowScore.title = QStringLiteral("低推荐任务-%1").arg(suffix);
    lowScore.importance = 1;
    Task uncategorized = highScore;
    uncategorized.id = -1;
    uncategorized.title = QStringLiteral("无项目任务-%1").arg(suffix);
    uncategorized.projectId = -1;
    uncategorized.categoryId = -1;
    uncategorized.importance = 3;

    QString error;
    TaskRepository repository;
    QVERIFY2(repository.save(highScore, &error), qPrintable(error));
    QVERIFY2(repository.save(lowScore, &error), qPrintable(error));
    QVERIFY2(repository.save(uncategorized, &error), qPrintable(error));

    FocusPage page;
    auto *projectFilter = page.findChild<QComboBox *>(
        QStringLiteral("focusProjectFilter"));
    auto *categoryFilter = page.findChild<QComboBox *>(
        QStringLiteral("focusCategoryFilter"));
    auto *taskCombo = page.findChild<QComboBox *>(
        QStringLiteral("focusTaskCombo"));
    QVERIFY(projectFilter != nullptr);
    QVERIFY(categoryFilter != nullptr);
    QVERIFY(taskCombo != nullptr);

    projectFilter->setCurrentIndex(projectFilter->findData(projectId));
    categoryFilter->setCurrentIndex(categoryFilter->findData(categoryId));
    QCOMPARE(taskCombo->count(), 3);
    QCOMPARE(taskCombo->itemData(1).toInt(), highScore.id);
    QCOMPARE(taskCombo->itemData(2).toInt(), lowScore.id);
    QVERIFY(taskCombo->itemText(1).contains(QStringLiteral("推荐分 100")));
    QVERIFY(taskCombo->itemText(2).contains(QStringLiteral("推荐分 20")));
    QCOMPARE(taskCombo->itemData(1, Qt::ForegroundRole).value<QColor>(),
             PriorityColors::recommendation(100));
    QCOMPARE(taskCombo->itemData(2, Qt::ForegroundRole).value<QColor>(),
             PriorityColors::recommendation(20));

    taskCombo->setCurrentIndex(taskCombo->findData(lowScore.id));
    projectFilter->setCurrentIndex(projectFilter->findData(-1));
    categoryFilter->setCurrentIndex(categoryFilter->findData(-1));
    QCOMPARE(taskCombo->count(), 2);
    QCOMPARE(taskCombo->currentData().toInt(), -1);
    QCOMPARE(taskCombo->itemData(1).toInt(), uncategorized.id);

    page.selectTask(highScore.id);
    QCOMPARE(projectFilter->currentData().toInt(), projectId);
    QCOMPARE(categoryFilter->currentData().toInt(), categoryId);
    QCOMPARE(taskCombo->currentData().toInt(), highScore.id);
}

void FocusPageTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(FocusPageTests)

#include "tst_FocusPage.moc"
