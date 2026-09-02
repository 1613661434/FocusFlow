#include "data/DatabaseManager.h"
#include "repositories/TaskRepository.h"
#include "repositories/TimerPresetRepository.h"
#include "views/FocusPage.h"
#include "widgets/PriorityColors.h"

#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QTest>
#include <QTimer>
#include <QUuid>

#include <algorithm>

class FocusPageTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void primaryButtonTracksTimerState();
    void taskFiltersShowScoresAndSortRecommendations();
    void taskSchemeAndTrayStatusFollowTheRunningTimer();
    void customSchemeRequiresConfirmationAndCanBeSaved();
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
    auto *statusLabel = page.findChild<QLabel *>(
        QStringLiteral("focusStatusLabel"));
    QVERIFY(primary);
    QVERIFY(stop);
    QVERIFY(statusLabel);
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
    QTRY_VERIFY_WITH_TIMEOUT(
        statusLabel->text().contains(QStringLiteral("已终止")), 500);
    QTRY_COMPARE_WITH_TIMEOUT(statusLabel->text(),
                              QStringLiteral("准备开始专注"), 4700);
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
    QCOMPARE(projectFilter->itemData(projectFilter->findData(projectId),
                                     Qt::ForegroundRole).value<QColor>(),
             QColor(QStringLiteral("#4F6EF7")));
    QCOMPARE(categoryFilter->itemData(categoryFilter->findData(categoryId),
                                      Qt::ForegroundRole).value<QColor>(),
             QColor(QStringLiteral("#C335B4")));
    QCOMPARE(projectFilter->palette().color(QPalette::Text),
             QColor(QStringLiteral("#4F6EF7")));
    QCOMPARE(categoryFilter->palette().color(QPalette::Text),
             QColor(QStringLiteral("#C335B4")));
    QCOMPARE(taskCombo->itemData(1, Qt::ForegroundRole).value<QColor>(),
             PriorityColors::recommendation(100));
    QCOMPARE(taskCombo->itemData(2, Qt::ForegroundRole).value<QColor>(),
             PriorityColors::recommendation(20));
    taskCombo->setCurrentIndex(1);
    QCOMPARE(taskCombo->palette().color(QPalette::Text),
             PriorityColors::recommendation(100));

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

void FocusPageTests::taskSchemeAndTrayStatusFollowTheRunningTimer()
{
    const QVector<TimerPreset> presets = TimerPresetRepository().findAll();
    const auto deepWork = std::find_if(
        presets.cbegin(), presets.cend(), [](const TimerPreset &preset) {
            return preset.name == QStringLiteral("深度工作");
        });
    QVERIFY(deepWork != presets.cend());

    Task task;
    task.title = QStringLiteral("托盘倒计时测试任务");
    task.timerPresetId = deepWork->id;
    QString error;
    QVERIFY2(TaskRepository().save(task, &error), qPrintable(error));

    FocusPage page;
    auto *presetCombo = page.findChild<QComboBox *>(
        QStringLiteral("focusPresetCombo"));
    auto *customMinutes = page.findChild<QSpinBox *>(
        QStringLiteral("focusCustomMinutes"));
    auto *primary = buttonForRole(page, QStringLiteral("primary"));
    auto *confirmCustom = page.findChild<QPushButton *>(
        QStringLiteral("confirmCustomMinutesButton"));
    QVERIFY(presetCombo != nullptr);
    QVERIFY(customMinutes != nullptr);
    QVERIFY(primary != nullptr);
    QVERIFY(confirmCustom != nullptr);

    page.selectTask(task.id);
    QCOMPARE(presetCombo->currentData().toInt(), deepWork->id);
    const int customIndex = presetCombo->findData(-2);
    QVERIFY(customIndex >= 0);
    presetCombo->setCurrentIndex(customIndex);
    QVERIFY(!customMinutes->isHidden());
    customMinutes->setValue(1);
    QVERIFY(!primary->isEnabled());
    QVERIFY(confirmCustom->isEnabled());
    confirmCustom->click();
    QVERIFY(primary->isEnabled());

    QSignalSpy traySpy(&page, &FocusPage::trayStatusChanged);
    primary->click();
    QVERIFY(!traySpy.isEmpty());
    QString status = traySpy.constLast().at(0).toString();
    QVERIFY(status.contains(QStringLiteral("专注中")));
    QVERIFY(status.contains(task.title));
    QVERIFY(status.contains(QStringLiteral("剩余 01:00")));

    primary->click();
    status = traySpy.constLast().at(0).toString();
    QVERIFY(status.contains(QStringLiteral("专注已暂停")));
}

void FocusPageTests::customSchemeRequiresConfirmationAndCanBeSaved()
{
    Task task;
    task.title = QStringLiteral("方案反馈测试任务");
    QString error;
    QVERIFY2(TaskRepository().save(task, &error), qPrintable(error));

    FocusPage page;
    page.resize(1000, 700);
    page.show();
    QCoreApplication::processEvents();
    auto *taskCombo = page.findChild<QComboBox *>(
        QStringLiteral("focusTaskCombo"));
    auto *presetCombo = page.findChild<QComboBox *>(
        QStringLiteral("focusPresetCombo"));
    auto *customMinutes = page.findChild<QSpinBox *>(
        QStringLiteral("focusCustomMinutes"));
    auto *shortBreakMinutes = page.findChild<QSpinBox *>(
        QStringLiteral("focusCustomShortBreakMinutes"));
    auto *longBreakMinutes = page.findChild<QSpinBox *>(
        QStringLiteral("focusCustomLongBreakMinutes"));
    auto *cycles = page.findChild<QSpinBox *>(
        QStringLiteral("focusCustomCycles"));
    auto *autoStartBreak = page.findChild<QCheckBox *>(
        QStringLiteral("focusCustomAutoStartBreak"));
    auto *autoStartFocus = page.findChild<QCheckBox *>(
        QStringLiteral("focusCustomAutoStartFocus"));
    auto *setTaskPreset = page.findChild<QPushButton *>(
        QStringLiteral("taskPresetButton"));
    auto *confirmCustom = page.findChild<QPushButton *>(
        QStringLiteral("confirmCustomMinutesButton"));
    auto *saveCustom = page.findChild<QPushButton *>(
        QStringLiteral("saveCustomPresetButton"));
    auto *statusLabel = page.findChild<QLabel *>(
        QStringLiteral("focusStatusLabel"));
    auto *customEditor = page.findChild<QWidget *>(
        QStringLiteral("focusCustomPresetEditor"));
    auto *primary = buttonForRole(page, QStringLiteral("primary"));
    QVERIFY(taskCombo != nullptr);
    QVERIFY(presetCombo != nullptr);
    QVERIFY(customMinutes != nullptr);
    QVERIFY(shortBreakMinutes != nullptr);
    QVERIFY(longBreakMinutes != nullptr);
    QVERIFY(cycles != nullptr);
    QVERIFY(autoStartBreak != nullptr);
    QVERIFY(autoStartFocus != nullptr);
    QVERIFY(setTaskPreset != nullptr);
    QVERIFY(confirmCustom != nullptr);
    QVERIFY(saveCustom != nullptr);
    QVERIFY(statusLabel != nullptr);
    QVERIFY(customEditor != nullptr);
    QVERIFY(primary != nullptr);
    QCOMPARE(saveCustom->text(), QStringLiteral("保存为方案"));
    QVERIFY(confirmCustom->minimumHeight() >= 40);
    QVERIFY(saveCustom->minimumWidth() >= 138);

    QCOMPARE(taskCombo->currentData().toInt(), -1);
    QVERIFY(!setTaskPreset->isEnabled());
    QVERIFY(setTaskPreset->toolTip().contains(QStringLiteral("关联任务")));

    page.selectTask(task.id);
    QVERIFY(setTaskPreset->isEnabled());
    setTaskPreset->click();
    QVERIFY(statusLabel->text().contains(QStringLiteral("默认专注方案")));
    QTRY_COMPARE_WITH_TIMEOUT(statusLabel->text(),
                              QStringLiteral("准备开始专注"), 3200);

    const int customIndex = presetCombo->findData(-2);
    QVERIFY(customIndex >= 0);
    presetCombo->setCurrentIndex(customIndex);
    QCoreApplication::processEvents();
    QVERIFY(customEditor->isVisible());
    presetCombo->setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(presetCombo->hasFocus());
    const int selectedPresetId = presetCombo->currentData().toInt();
    QTest::mouseClick(&page, Qt::LeftButton, Qt::NoModifier,
                      QPoint(4, 4));
    QVERIFY(!presetCombo->hasFocus());
    QCOMPARE(presetCombo->currentData().toInt(), selectedPresetId);
    for (QLabel *label : page.findChildren<QLabel *>()) {
        QVERIFY(label->text() != QStringLiteral("自定义本次方案"));
    }
    const int confirmBottom =
        confirmCustom->mapTo(customEditor,
                             QPoint(0, confirmCustom->height())).y();
    const int saveBottom =
        saveCustom->mapTo(customEditor,
                          QPoint(0, saveCustom->height())).y();
    QVERIFY(confirmBottom < customEditor->height());
    QVERIFY(saveBottom < customEditor->height());
    QVERIFY(cycles->width() >= 190);
    customMinutes->setValue(7);
    shortBreakMinutes->setValue(2);
    longBreakMinutes->setValue(11);
    cycles->setValue(3);
    autoStartBreak->setChecked(true);
    autoStartFocus->setChecked(true);
    QVERIFY(confirmCustom->isEnabled());
    QVERIFY(!saveCustom->isEnabled());
    QVERIFY(!primary->isEnabled());
    customMinutes->setFocus();
    QCoreApplication::processEvents();
    QTest::mouseClick(&page, Qt::LeftButton, Qt::NoModifier,
                      QPoint(4, 4));
    QVERIFY(!customMinutes->hasFocus());
    confirmCustom->click();
    QVERIFY(primary->isEnabled());
    QVERIFY(!confirmCustom->isEnabled());
    QVERIFY(saveCustom->isEnabled());

    TimerPreset existingName;
    existingName.id = -1;
    existingName.name = QStringLiteral("我的专注方案");
    existingName.focusMinutes = 25;
    existingName.shortBreakMinutes = 5;
    existingName.longBreakMinutes = 15;
    existingName.cyclesBeforeLongBreak = 4;
    QVERIFY2(TimerPresetRepository().save(existingName, &error),
             qPrintable(error));
    TimerPreset existingNumberedName = existingName;
    existingNumberedName.id = -1;
    existingNumberedName.name = QStringLiteral("我的专注方案2");
    QVERIFY2(TimerPresetRepository().save(existingNumberedName, &error),
             qPrintable(error));

    const QString savedName = QStringLiteral("我的专注方案3");
    QString suggestedName;
    QSignalSpy presetsChangedSpy(&page, &FocusPage::presetsChanged);
    QTimer::singleShot(0, [&suggestedName] {
        if (auto *dialog = qobject_cast<QInputDialog *>(
                QApplication::activeModalWidget())) {
            suggestedName = dialog->textValue();
            dialog->accept();
        }
    });
    saveCustom->click();
    QCOMPARE(suggestedName, savedName);
    QCOMPARE(presetsChangedSpy.count(), 1);
    const QVector<TimerPreset> presets = TimerPresetRepository().findAll();
    const auto saved = std::find_if(
        presets.cbegin(), presets.cend(), [&savedName](const TimerPreset &preset) {
            return preset.name == savedName;
        });
    QVERIFY(saved != presets.cend());
    QCOMPARE(saved->focusMinutes, 7);
    QCOMPARE(saved->shortBreakMinutes, 2);
    QCOMPARE(saved->longBreakMinutes, 11);
    QCOMPARE(saved->cyclesBeforeLongBreak, 3);
    QVERIFY(saved->autoStartBreak);
    QVERIFY(saved->autoStartFocus);
    QVERIFY(!saved->isBuiltIn);

    taskCombo->setCurrentIndex(taskCombo->findData(-1));
    QVERIFY(!setTaskPreset->isEnabled());
    QVERIFY(setTaskPreset->toolTip().contains(QStringLiteral("关联任务")));
}

void FocusPageTests::cleanupTestCase()
{
    DatabaseManager::instance().database().close();
    QDir(dataDirectory_).removeRecursively();
}

QTEST_MAIN(FocusPageTests)

#include "tst_FocusPage.moc"
