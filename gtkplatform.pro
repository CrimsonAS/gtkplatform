TARGET = qgtk

CONFIG -= release
CONFIG += debug
QT += core-private gui-private platformsupport-private widgets

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            qgtkintegration.cpp \
            qgtkbackingstore.cpp \
            qgtkscreen.cpp \
            qgtkwindow.cpp \
            qgtkwindow_keyboard.cpp \
            qgtkwindow_mouse.cpp \
            qgtkwindow_touch.cpp \
            qgtktheme.cpp \
            qgtksystemtrayicon.cpp \
            qgtkmenubar.cpp \
            qgtkmenu.cpp \
            qgtkmenuitem.cpp \
            qgtkhelpers.cpp \
            qgtk3dialoghelpers.cpp \
            qgtkopenglcontext.cpp \
            qgtkstyle.cpp
            #qgtkclipboard.cpp

HEADERS =   qgtkintegration.h \
            qgtkbackingstore.h \
            qgtkscreen.h \
            qgtkwindow.h \
            qgtktheme.h \
            qgtksystemtrayicon.h \
            qgtkmenubar.h \
            qgtkmenu.h \
            qgtkmenuitem.h \
            qgtkhelpers.h \
            qgtk3dialoghelpers.h \
            qgtkopenglcontext.h \
            qgtkstyle.h
            #qgtkclipboard.h

CONFIG += qpa/genericunixfontdatabase

CONFIG += link_pkgconfig
PKGCONFIG_PRIVATE += gdk-3.0 gtk+-3.0

# for GL
PKGCONFIG_PRIVATE += egl

CONFIG += no_keywords

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QGtkIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
