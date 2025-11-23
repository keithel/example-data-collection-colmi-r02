# Colmi R02 Qt C++ Data Explorer App
#
# Copyright (C) 2025 Keith Kyzivat <keithel @ github>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

cmake_minimum_required(VERSION 3.16)

project(r02_data_explorer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Bluetooth Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appR02DataExplorer
    src/main.cpp
)

set(QML_IMPORT_PATH ${CMAKE_CURRENT_BINARY_DIR}/R02DataExplorer )

qt_add_qml_module(appR02DataExplorer
    URI R02DataExplorer
    VERSION 1.0
    QML_FILES
        Main.qml
        AboutPage.qml
        AccelerometerDisplay.qml
    SOURCES
        src/ringconnector.h
        src/ringconnector.cpp
        src/systemtray.h
        src/systemtray.cpp
    RESOURCES
        images/qt-logo.svg
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appR02DataExplorer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appR02DataExplorer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appR02DataExplorer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(appR02DataExplorer
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Bluetooth
        Qt6::Widgets
)
target_link_libraries(appR02DataExplorer PRIVATE Qt6::Core)

include(GNUInstallDirs)
install(TARGETS appR02DataExplorer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
