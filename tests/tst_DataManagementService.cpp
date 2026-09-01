#include "data/DatabaseManager.h"
#include "services/DataManagementService.h"
#include "services/SoundStorageService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

class DataManagementServiceTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void backupAndCsvExports();
    void prepareValidatedRestore();
    void clearFocusStatisticsKeepsTasks();
    void managedSoundStorageRemovesOnlyUnusedCopies();
    void cleanupTestCase();

private:
    QString dataDirectory_;
};

void DataManagementServiceTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlowTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("DataManagement-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));

    QVERIFY2(DatabaseManager::instance().initialize(),
             qPrintable(DatabaseManager::instance().lastError()));
    dataDirectory_ = QFileInfo(DatabaseManager::instance().databasePath()).absolutePath();

    QSqlQuery task(DatabaseManager::instance().database());
    QVERIFY(task.exec(QStringLiteral(R"(
        INSERT INTO tasks(title, description, importance, status)
        VALUES('整理资料, 然后归档', '包含"引号"和换行
内容', 4, 'pending')
    )")));

    QSqlQuery focus(DatabaseManager::instance().database());
    QVERIFY(focus.exec(QStringLiteral(R"(
        INSERT INTO focus_sessions(
            task_id, session_type, status, start_time, end_time,
            planned_seconds, actual_seconds
        ) VALUES(1, 'focus', 'completed', '2026-09-01T09:00:00',
                 '2026-09-01T09:25:00', 1500, 1500)
    )")));
}

void DataManagementServiceTests::backupAndCsvExports()
{
    QTemporaryDir output;
    QVERIFY(output.isValid());

    DataManagementService service;
    QString error;
    const QString backupPath = output.filePath(QStringLiteral("backup.db"));
    QVERIFY2(service.backupDatabase(backupPath, &error), qPrintable(error));
    QVERIFY(QFileInfo(backupPath).size() > 0);

    const QString tasksPath = output.filePath(QStringLiteral("tasks.csv"));
    QVERIFY2(service.exportTasksCsv(tasksPath, &error), qPrintable(error));
    QFile tasksFile(tasksPath);
    QVERIFY(tasksFile.open(QIODevice::ReadOnly));
    const QByteArray tasksCsv = tasksFile.readAll();
    QVERIFY(tasksCsv.contains("FocusFlow") == false);
    QVERIFY(tasksCsv.contains("\"\""));
    QVERIFY(tasksCsv.contains("\xe6\x95\xb4\xe7\x90\x86\xe8\xb5\x84\xe6\x96\x99"));

    const QString focusPath = output.filePath(QStringLiteral("focus.csv"));
    QVERIFY2(service.exportFocusSessionsCsv(focusPath, &error), qPrintable(error));
    QFile focusFile(focusPath);
    QVERIFY(focusFile.open(QIODevice::ReadOnly));
    const QByteArray focusCsv = focusFile.readAll();
    QVERIFY(focusCsv.contains("1500"));
}

void DataManagementServiceTests::prepareValidatedRestore()
{
    QTemporaryDir output;
    QVERIFY(output.isValid());

    DataManagementService service;
    QString error;
    const QString backupPath = output.filePath(QStringLiteral("restore.db"));
    QVERIFY2(service.backupDatabase(backupPath, &error), qPrintable(error));

    QString recoveryPath;
    QVERIFY2(service.prepareRestore(backupPath, &recoveryPath, &error), qPrintable(error));
    QVERIFY(QFileInfo::exists(recoveryPath));
    QVERIFY(QFileInfo::exists(
        QDir(dataDirectory_).filePath(QStringLiteral("focusflow.restore-pending.db"))));
}

void DataManagementServiceTests::clearFocusStatisticsKeepsTasks()
{
    DataManagementService service;
    QString error;
    QVERIFY2(service.clearFocusStatistics(&error), qPrintable(error));

    QSqlQuery focusCount(DatabaseManager::instance().database());
    QVERIFY(focusCount.exec(QStringLiteral("SELECT COUNT(*) FROM focus_sessions")));
    QVERIFY(focusCount.next());
    QCOMPARE(focusCount.value(0).toInt(), 0);

    QSqlQuery taskCount(DatabaseManager::instance().database());
    QVERIFY(taskCount.exec(QStringLiteral("SELECT COUNT(*) FROM tasks")));
    QVERIFY(taskCount.next());
    QVERIFY(taskCount.value(0).toInt() > 0);
}

void DataManagementServiceTests::managedSoundStorageRemovesOnlyUnusedCopies()
{
    QTemporaryDir sources;
    QVERIFY(sources.isValid());

    const QString firstSource = sources.filePath(QStringLiteral("first.wav"));
    const QString secondSource = sources.filePath(QStringLiteral("second.mp3"));
    for (const QString &path : {firstSource, secondSource}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("test-audio"), qint64(10));
    }

    SoundStorageService storage;
    QString error;
    const QString firstManaged =
        storage.install(firstSource, QStringLiteral("focus"), &error);
    QVERIFY2(!firstManaged.isEmpty(), qPrintable(error));
    const QString secondManaged =
        storage.install(secondSource, QStringLiteral("break"), &error);
    QVERIFY2(!secondManaged.isEmpty(), qPrintable(error));
    QVERIFY(storage.isManagedPath(firstManaged));
    QVERIFY(storage.isManagedPath(secondManaged));
    QVERIFY(QFileInfo::exists(firstManaged));
    QVERIFY(QFileInfo::exists(secondManaged));

    QCOMPARE(storage.install(secondManaged, QStringLiteral("focus"), &error),
             secondManaged);
    QVERIFY(storage.removeUnused({secondManaged, secondManaged}).isEmpty());
    QVERIFY(!QFileInfo::exists(firstManaged));
    QVERIFY(QFileInfo::exists(secondManaged));
    QVERIFY(QFileInfo::exists(firstSource));
    QVERIFY(QFileInfo::exists(secondSource));

    QVERIFY(storage.removeUnused({}).isEmpty());
    QVERIFY(!QFileInfo::exists(secondManaged));
}

void DataManagementServiceTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(DataManagementServiceTests)

#include "tst_DataManagementService.moc"
