cmake_minimum_required(VERSION 3.21)

project(FocusFlow VERSION 0.1.0 LANGUAGES CXX)

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
)

qt_standard_project_setup()

qt_add_executable(FocusFlow WIN32
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
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
