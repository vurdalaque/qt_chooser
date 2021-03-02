cmake_minimum_required(VERSION 3.19)

project(qt_chooser VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDART 11)
set(CMAKE_CXX_STANDART_REQUIRED True)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
# set(CMAKE_FIND_DEBUG_MODE 1)
list(APPEND CMAKE_PREFIX_PATH $ENV{ISS_LIB_HOME}/qt/5.11.3/qtbase)

find_package(Qt5 5.11 REQUIRED COMPONENTS Core Gui Widgets AxContainer)
add_compile_definitions(QT_DEPRECATED_WARNINGS)

include_directories(
	${Qt5Widgets_INCLUDE_DIRS}
	${Qt5Core_PRIVATE_INCLUDE_DIRS}
	)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

add_executable(${CMAKE_PROJECT_NAME} qrc/resources.qrc)

add_subdirectory(${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
	qt_chooser
	Qt5::Core
	Qt5::AxContainer
	Qt5::Widgets
	)

