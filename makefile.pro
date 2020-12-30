# vi: fenc=utf8
TEMPLATE = app
QT += widgets gui core core-private axcontainer
TARGET = ../qt_chooser


# CONFIG -= qt debug_and_release debug_and_release_target
CONFIG -= debug_and_release debug_and_release_target
# CONFIG += console debug
CONFIG += release
# CONFIG += console

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
			$$files(src/*.cpp)

HEADERS += \
			$$files(src/*.h)

RESOURCES += \
			$$files(qrc/*.qrc)

FORMS += $$files(ui/*.ui)

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
} else {
    DESTDIR = build/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.u

win32: QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings 
win32: QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings 
win32: QMAKE_CFLAGS -= -Zc:strictStrings 
win32: QMAKE_CXXFLAGS -= -Zc:strictStrings 
QMAKE_CXXFLAGS_RELEASE += /std:c++17

