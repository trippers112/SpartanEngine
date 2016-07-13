#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:21:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Editor
TEMPLATE = app


SOURCES += main.cpp\
        editor.cpp \
    DirectusQTHelper.cpp \
    DirectusPlayButton.cpp \
    AboutDialog.cpp \
    DirectusDirExplorer.cpp \
    DirectusFileExplorer.cpp \
    Directus3D.cpp \
    DirectusConsole.cpp \
    DirectusInspector.cpp \
    DirectusHierarchy.cpp \
    DirectusTransform.cpp \
    DirectusAdjustLabel.cpp

HEADERS  += editor.h \
    DirectusQTHelper.h \
    DirectusPlayButton.h \
    AboutDialog.h \
    DirectusDirExplorer.h \
    DirectusFileExplorer.h \
    DirectusConsole.h \
    Directus3D.h \
    DirectusInspector.h \
    DirectusHierarchy.h \
    DirectusTransform.h \
    DirectusAdjustLabel.h

FORMS    += editor.ui \
    AboutDialog.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Binaries/ -lDirectus3d
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Binaries/ -lDirectus3d

INCLUDEPATH += $$PWD/../Directus3D
DEPENDPATH += $$PWD/../Directus3D

DISTFILES +=

ParentDirectory = D:\Projects\Directus3D\Binaries

DESTDIR = "$$ParentDirectory"
RCC_DIR = "$$ParentDirectory\RCCFiles"
UI_DIR = "$$ParentDirectory\UICFiles"
MOC_DIR = "$$ParentDirectory\MOCFiles"
OBJECTS_DIR = "$$ParentDirectory\ObjFiles"

RESOURCES += \
    Images/images.qrc