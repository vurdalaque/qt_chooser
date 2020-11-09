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

HEADERS += \
			wmi.h \
			helper.h \
			settings.h \
			statelabel.h \
			service.h \
			process.h


SOURCES += \
			helper.cpp \
			tooling.cpp \
			wmi.cpp \
			stylesheet.cpp \
			settings.cpp \
			service.cpp \
			statelabel.cpp \
			process.cpp \
			main.cpp

FORMS += \
			symlinkframe.ui \
			serviceframe.ui \
			moveframe.ui \
			processframe.ui \
			processwidget.ui


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
