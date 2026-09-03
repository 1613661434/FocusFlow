cmake_minimum_required(VERSION 3.21)

project(FocusFlow VERSION 0.1.39 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Widgets
    Sql
    Multimedia
    Charts
    Network
    Test
)

qt_standard_project_setup()

qt_add_executable(FocusFlow WIN32
    resources/resources.qrc
    resources/focusflow.rc
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/SingleInstanceGuard.cpp
    src/app/SingleInstanceGuard.h
    src/data/DatabaseManager.cpp
    src/data/DatabaseManager.h
    src/models/Task.h
    src/models/Project.h
    src/models/TimerSettings.h
    src/models/TimerPreset.h
    src/repositories/FocusRepository.cpp
    src/repositories/FocusRepository.h
    src/repositories/AnalyticsRepository.cpp
    src/repositories/AnalyticsRepository.h
    src/repositories/SettingsRepository.cpp
    src/repositories/SettingsRepository.h
    src/repositories/ProjectRepository.cpp
    src/repositories/ProjectRepository.h
    src/repositories/TaskRepository.cpp
    src/repositories/TaskRepository.h
    src/repositories/TimerPresetRepository.cpp
    src/repositories/TimerPresetRepository.h
    src/services/FocusTimer.cpp
    src/services/FocusTimer.h
    src/services/DataManagementService.cpp
    src/services/DataManagementService.h
    src/services/NotificationSoundPlayer.cpp
    src/services/NotificationSoundPlayer.h
    src/services/SoundStorageService.cpp
    src/services/SoundStorageService.h
    src/services/PriorityService.cpp
    src/services/PriorityService.h
    src/views/TaskDialog.cpp
    src/views/TaskDialog.h
    src/views/TimerPresetDialog.cpp
    src/views/TimerPresetDialog.h
    src/views/AboutDialog.cpp
    src/views/AboutDialog.h
    src/views/TaskPage.cpp
    src/views/TaskPage.h
    src/views/FocusPage.cpp
    src/views/FocusPage.h
    src/views/SettingsPage.cpp
    src/views/SettingsPage.h
    src/views/ProjectPage.cpp
    src/views/ProjectPage.h
    src/views/DashboardPage.cpp
    src/views/DashboardPage.h
    src/views/StatisticsPage.cpp
    src/views/StatisticsPage.h
    src/widgets/ChineseCalendarWidget.cpp
    src/widgets/ChineseCalendarWidget.h
    src/widgets/FocusAwareComboBox.cpp
    src/widgets/FocusAwareComboBox.h
    src/widgets/FocusAwareSpinBox.cpp
    src/widgets/FocusAwareSpinBox.h
    src/widgets/FocusAwareSlider.cpp
    src/widgets/FocusAwareSlider.h
    src/widgets/FocusAwareTableWidget.cpp
    src/widgets/FocusAwareTableWidget.h
    src/widgets/PriorityColors.h
    src/widgets/StatusColors.h
    src/widgets/ColoredComboBox.h
    src/widgets/TextOnlyMenu.cpp
    src/widgets/TextOnlyMenu.h
    src/widgets/ClearSelectionOnBlankClick.cpp
    src/widgets/ClearSelectionOnBlankClick.h
    src/widgets/SortKeyTableWidgetItem.h
)

target_include_directories(FocusFlow PRIVATE src)

target_link_libraries(FocusFlow PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Multimedia
    Qt6::Charts
    Qt6::Network
)

if(WIN32)
    target_link_libraries(FocusFlow PRIVATE dwmapi)
endif()

set_target_properties(FocusFlow PROPERTIES
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS FocusFlow
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(SingleInstanceGuardTests
        tests/tst_SingleInstanceGuard.cpp
        src/app/SingleInstanceGuard.cpp
        src/app/SingleInstanceGuard.h
    )
    target_include_directories(SingleInstanceGuardTests PRIVATE src)
    target_link_libraries(SingleInstanceGuardTests PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Test
    )
    add_test(NAME SingleInstanceGuardTests COMMAND SingleInstanceGuardTests)

    qt_add_executable(FocusTimerTests
        tests/tst_FocusTimer.cpp
        src/services/FocusTimer.cpp
        src/services/FocusTimer.h
    )
    target_include_directories(FocusTimerTests PRIVATE src)
    target_link_libraries(FocusTimerTests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME FocusTimerTests COMMAND FocusTimerTests)

    qt_add_executable(FocusPageTests
        tests/tst_FocusPage.cpp
        src/views/FocusPage.cpp
        src/views/FocusPage.h
        src/views/TimerPresetDialog.cpp
        src/views/TimerPresetDialog.h
        src/data/DatabaseManager.cpp
        src/data/DatabaseManager.h
        src/repositories/FocusRepository.cpp
        src/repositories/FocusRepository.h
        src/repositories/SettingsRepository.cpp
        src/repositories/SettingsRepository.h
        src/repositories/TaskRepository.cpp
        src/repositories/TaskRepository.h
        src/repositories/TimerPresetRepository.cpp
        src/repositories/TimerPresetRepository.h
        src/services/FocusTimer.cpp
        src/services/FocusTimer.h
        src/services/NotificationSoundPlayer.cpp
        src/services/NotificationSoundPlayer.h
        src/services/PriorityService.cpp
        src/services/PriorityService.h
        src/widgets/PriorityColors.h
        src/widgets/StatusColors.h
        src/widgets/ColoredComboBox.h
        src/widgets/FocusAwareSpinBox.cpp
        src/widgets/FocusAwareSpinBox.h
        src/models/Task.h
        src/models/TimerSettings.h
        src/models/TimerPreset.h
    )
    target_include_directories(FocusPageTests PRIVATE src)
    target_link_libraries(FocusPageTests PRIVATE
        Qt6::Widgets
        Qt6::Sql
        Qt6::Multimedia
        Qt6::Test
    )
    add_test(NAME FocusPageTests COMMAND FocusPageTests)

    qt_add_executable(PriorityServiceTests
        tests/tst_PriorityService.cpp
        src/services/PriorityService.cpp
        src/services/PriorityService.h
        src/models/Task.h
    )
    target_include_directories(PriorityServiceTests PRIVATE src)
    target_link_libraries(PriorityServiceTests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME PriorityServiceTests COMMAND PriorityServiceTests)

    qt_add_executable(TimerPresetRepositoryTests
        tests/tst_TimerPresetRepository.cpp
        src/data/DatabaseManager.cpp
        src/data/DatabaseManager.h
        src/repositories/TaskRepository.cpp
        src/repositories/TaskRepository.h
        src/repositories/TimerPresetRepository.cpp
        src/repositories/TimerPresetRepository.h
        src/models/Task.h
        src/models/TimerPreset.h
    )
    target_include_directories(TimerPresetRepositoryTests PRIVATE src)
    target_link_libraries(TimerPresetRepositoryTests PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
    )
    add_test(NAME TimerPresetRepositoryTests COMMAND TimerPresetRepositoryTests)

    qt_add_executable(DataManagementServiceTests
        tests/tst_DataManagementService.cpp
        src/data/DatabaseManager.cpp
        src/data/DatabaseManager.h
        src/services/DataManagementService.cpp
        src/services/DataManagementService.h
        src/services/SoundStorageService.cpp
        src/services/SoundStorageService.h
    )
    target_include_directories(DataManagementServiceTests PRIVATE src)
    target_link_libraries(DataManagementServiceTests PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
    )
    add_test(NAME DataManagementServiceTests COMMAND DataManagementServiceTests)

    qt_add_executable(ChineseCalendarWidgetTests
        tests/tst_ChineseCalendarWidget.cpp
        src/widgets/ChineseCalendarWidget.cpp
        src/widgets/ChineseCalendarWidget.h
    )
    target_include_directories(ChineseCalendarWidgetTests PRIVATE src)
    target_link_libraries(ChineseCalendarWidgetTests PRIVATE
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME ChineseCalendarWidgetTests COMMAND ChineseCalendarWidgetTests)

    qt_add_executable(QtLocalizationTests
        tests/tst_QtLocalization.cpp
    )
    target_link_libraries(QtLocalizationTests PRIVATE
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME QtLocalizationTests COMMAND QtLocalizationTests)

    qt_add_executable(FocusAwareSpinBoxTests
        tests/tst_FocusAwareSpinBox.cpp
        src/widgets/FocusAwareComboBox.cpp
        src/widgets/FocusAwareComboBox.h
        src/widgets/FocusAwareSpinBox.cpp
        src/widgets/FocusAwareSpinBox.h
        src/widgets/ColoredComboBox.h
        src/widgets/FocusAwareSlider.cpp
        src/widgets/FocusAwareSlider.h
        src/widgets/FocusAwareTableWidget.cpp
        src/widgets/FocusAwareTableWidget.h
    )
    target_include_directories(FocusAwareSpinBoxTests PRIVATE src)
    target_link_libraries(FocusAwareSpinBoxTests PRIVATE
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME FocusAwareSpinBoxTests COMMAND FocusAwareSpinBoxTests)

    qt_add_executable(TaskDialogTests
        tests/tst_TaskDialog.cpp
        src/views/TaskDialog.cpp
        src/views/TaskDialog.h
        src/widgets/ChineseCalendarWidget.cpp
        src/widgets/ChineseCalendarWidget.h
        src/widgets/FocusAwareSpinBox.cpp
        src/widgets/FocusAwareSpinBox.h
        src/models/Task.h
        src/models/TimerPreset.h
    )
    target_include_directories(TaskDialogTests PRIVATE src)
    target_link_libraries(TaskDialogTests PRIVATE
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME TaskDialogTests COMMAND TaskDialogTests)

    qt_add_executable(AboutDialogTests
        resources/resources.qrc
        tests/tst_AboutDialog.cpp
        src/views/AboutDialog.cpp
        src/views/AboutDialog.h
    )
    target_include_directories(AboutDialogTests PRIVATE src)
    target_link_libraries(AboutDialogTests PRIVATE
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME AboutDialogTests COMMAND AboutDialogTests)

    qt_add_executable(DashboardPageTests
        tests/tst_DashboardPage.cpp
        src/views/DashboardPage.cpp
        src/views/DashboardPage.h
        src/views/TaskPage.cpp
        src/views/TaskPage.h
        src/views/TaskDialog.cpp
        src/views/TaskDialog.h
        src/views/ProjectPage.cpp
        src/views/ProjectPage.h
        src/views/StatisticsPage.cpp
        src/views/StatisticsPage.h
        src/data/DatabaseManager.cpp
        src/data/DatabaseManager.h
        src/repositories/AnalyticsRepository.cpp
        src/repositories/AnalyticsRepository.h
        src/repositories/FocusRepository.cpp
        src/repositories/FocusRepository.h
        src/repositories/ProjectRepository.cpp
        src/repositories/ProjectRepository.h
        src/repositories/SettingsRepository.cpp
        src/repositories/SettingsRepository.h
        src/repositories/TaskRepository.cpp
        src/repositories/TaskRepository.h
        src/repositories/TimerPresetRepository.cpp
        src/repositories/TimerPresetRepository.h
        src/services/PriorityService.cpp
        src/services/PriorityService.h
        src/widgets/ClearSelectionOnBlankClick.cpp
        src/widgets/ClearSelectionOnBlankClick.h
        src/widgets/ChineseCalendarWidget.cpp
        src/widgets/ChineseCalendarWidget.h
        src/widgets/FocusAwareSpinBox.cpp
        src/widgets/FocusAwareSpinBox.h
        src/widgets/PriorityColors.h
        src/widgets/StatusColors.h
        src/widgets/ColoredComboBox.h
        src/widgets/TextOnlyMenu.cpp
        src/widgets/TextOnlyMenu.h
        src/widgets/SortKeyTableWidgetItem.h
        src/models/Task.h
        src/models/TimerPreset.h
    )
    target_include_directories(DashboardPageTests PRIVATE src)
    target_link_libraries(DashboardPageTests PRIVATE
        Qt6::Widgets
        Qt6::Sql
        Qt6::Charts
        Qt6::Test
    )
    if(WIN32)
        target_link_libraries(DashboardPageTests PRIVATE dwmapi)
    endif()
    add_test(NAME DashboardPageTests COMMAND DashboardPageTests)
endif()
