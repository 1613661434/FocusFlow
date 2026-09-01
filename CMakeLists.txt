cmake_minimum_required(VERSION 3.21)

project(FocusFlow VERSION 0.1.1 LANGUAGES CXX)

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
    Test
)

qt_standard_project_setup()

qt_add_executable(FocusFlow WIN32
    resources/resources.qrc
    resources/focusflow.rc
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/data/DatabaseManager.cpp
    src/data/DatabaseManager.h
    src/models/Task.h
    src/models/Project.h
    src/models/TimerSettings.h
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
    src/services/FocusTimer.cpp
    src/services/FocusTimer.h
    src/services/DataManagementService.cpp
    src/services/DataManagementService.h
    src/services/NotificationSoundPlayer.cpp
    src/services/NotificationSoundPlayer.h
    src/services/PriorityService.cpp
    src/services/PriorityService.h
    src/views/TaskDialog.cpp
    src/views/TaskDialog.h
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
)

target_include_directories(FocusFlow PRIVATE src)

target_link_libraries(FocusFlow PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Multimedia
    Qt6::Charts
)

set_target_properties(FocusFlow PROPERTIES
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS FocusFlow
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(FocusTimerTests
        tests/tst_FocusTimer.cpp
        src/services/FocusTimer.cpp
        src/services/FocusTimer.h
    )
    target_include_directories(FocusTimerTests PRIVATE src)
    target_link_libraries(FocusTimerTests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME FocusTimerTests COMMAND FocusTimerTests)

    qt_add_executable(PriorityServiceTests
        tests/tst_PriorityService.cpp
        src/services/PriorityService.cpp
        src/services/PriorityService.h
        src/models/Task.h
    )
    target_include_directories(PriorityServiceTests PRIVATE src)
    target_link_libraries(PriorityServiceTests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME PriorityServiceTests COMMAND PriorityServiceTests)

    qt_add_executable(DataManagementServiceTests
        tests/tst_DataManagementService.cpp
        src/data/DatabaseManager.cpp
        src/data/DatabaseManager.h
        src/services/DataManagementService.cpp
        src/services/DataManagementService.h
    )
    target_include_directories(DataManagementServiceTests PRIVATE src)
    target_link_libraries(DataManagementServiceTests PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
    )
    add_test(NAME DataManagementServiceTests COMMAND DataManagementServiceTests)
endif()
