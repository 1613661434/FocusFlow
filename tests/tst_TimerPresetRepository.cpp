#include "data/DatabaseManager.h"
#include "repositories/TaskRepository.h"
#include "repositories/TimerPresetRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTest>
#include <QUuid>

#include <algorithm>

class TimerPresetRepositoryTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void seedsUsefulSchemesAndMaintainsOneDefault();
    void taskBindingFallsBackWhenSchemeIsDeleted();
    void cleanupTestCase();

private:
    QString dataDirectory_;
};

void TimerPresetRepositoryTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlowTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("TimerPresetRepository-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QVERIFY2(DatabaseManager::instance().initialize(),
             qPrintable(DatabaseManager::instance().lastError()));
    dataDirectory_ = QFileInfo(
        DatabaseManager::instance().databasePath()).absolutePath();
}

void TimerPresetRepositoryTests::seedsUsefulSchemesAndMaintainsOneDefault()
{
    TimerPresetRepository repository;
    const QVector<TimerPreset> presets = repository.findAll();
    QVERIFY(presets.size() >= 4);
    QCOMPARE(std::count_if(presets.cbegin(), presets.cend(),
                           [](const TimerPreset &preset) {
                               return preset.isDefault;
                           }),
             1);
    QCOMPARE(std::count_if(presets.cbegin(), presets.cend(),
                           [](const TimerPreset &preset) {
                               return preset.isBuiltIn;
                           }),
             4);
    QVERIFY(std::any_of(presets.cbegin(), presets.cend(),
                        [](const TimerPreset &preset) {
                            return preset.name == QStringLiteral("深度工作")
                                   && preset.focusMinutes == 50
                                   && preset.isBuiltIn;
                        }));

    const auto classicIt = std::find_if(
        presets.cbegin(), presets.cend(), [](const TimerPreset &preset) {
            return preset.name == QStringLiteral("经典番茄钟");
        });
    QVERIFY(classicIt != presets.cend());
    TimerPreset changedBuiltIn = *classicIt;
    changedBuiltIn.focusMinutes = 30;
    QString error;
    QVERIFY(!repository.save(changedBuiltIn, &error));
    QVERIFY(error.contains(QStringLiteral("内置预设")));
    error.clear();
    QVERIFY(!repository.remove(classicIt->id, &error));
    QVERIFY(error.contains(QStringLiteral("内置预设")));

    TimerPreset custom;
    custom.id = -1;
    custom.name = QStringLiteral("测试方案");
    custom.focusMinutes = 40;
    custom.shortBreakMinutes = 8;
    custom.longBreakMinutes = 20;
    custom.cyclesBeforeLongBreak = 3;
    error.clear();
    QVERIFY2(repository.save(custom, &error), qPrintable(error));
    QVERIFY(custom.id > 0);
    QVERIFY(!custom.isBuiltIn);
    QVERIFY2(repository.setDefault(custom.id, &error), qPrintable(error));
    QCOMPARE(repository.defaultPreset().id, custom.id);
}

void TimerPresetRepositoryTests::taskBindingFallsBackWhenSchemeIsDeleted()
{
    TimerPresetRepository presetRepository;
    QVector<TimerPreset> presets = presetRepository.findAll();
    const auto customIt = std::find_if(
        presets.cbegin(), presets.cend(), [](const TimerPreset &preset) {
            return preset.name == QStringLiteral("测试方案");
        });
    QVERIFY(customIt != presets.cend());

    Task task;
    task.title = QStringLiteral("绑定方案测试任务");
    task.timerPresetId = customIt->id;
    QString error;
    TaskRepository taskRepository;
    QVERIFY2(taskRepository.save(task, &error), qPrintable(error));
    QCOMPARE(taskRepository.findById(task.id).timerPresetId, customIt->id);

    const auto replacementIt = std::find_if(
        presets.cbegin(), presets.cend(), [customIt](const TimerPreset &preset) {
            return preset.id != customIt->id;
        });
    QVERIFY(replacementIt != presets.cend());
    QVERIFY2(presetRepository.setDefault(replacementIt->id, &error),
             qPrintable(error));
    QVERIFY2(presetRepository.remove(customIt->id, &error), qPrintable(error));
    QCOMPARE(taskRepository.findById(task.id).timerPresetId, -1);
}

void TimerPresetRepositoryTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(TimerPresetRepositoryTests)

#include "tst_TimerPresetRepository.moc"
