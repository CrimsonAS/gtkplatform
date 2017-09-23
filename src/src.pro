TARGET = qgtk

CONFIG -= release
CONFIG += debug
QT += core-private gui-private widgets

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 8): {
    QT += platformsupport-private
} else {
    QT += fontdatabase_support_private glx_support_private egl_support_private service_support_private theme_support_private
}

equals(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 7): {
    # qhighdpi has some bugs with this in at least 5.7.
    DEFINES += QT_NO_FOREACH
}

SOURCES =   main.cpp \
            qgtkintegration.cpp \
            qgtkbackingstore.cpp \
            qgtkscreen.cpp \
            qgtkwindow.cpp \
            qgtkwindow_keyboard.cpp \
            qgtkwindow_mouse.cpp \
            qgtkwindow_touch.cpp \
            qgtkwindow_render.cpp \
            qgtktheme.cpp \
            qgtksystemtrayicon.cpp \
            qgtkmenubar.cpp \
            qgtkmenu.cpp \
            qgtkmenuitem.cpp \
            qgtkhelpers.cpp \
            qgtk3dialoghelpers.cpp \
            qgtkopenglcontext.cpp \
            qgtkopenglcontext_wayland.cpp \
            qgtkopenglcontext_x11.cpp \
            qgtkcursor.cpp \
            qgtkeventdispatcher.cpp \
            qgtkclipboard.cpp \
            qgtkservices.cpp

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
            qgtkrefptr.h \
            qgtkcursor.h \
            qgtkeventdispatcher.h \
            qgtkclipboard.h \
            qgtkservices.h

# CSystrace
DEFINES += DISABLE_TRACE_CODE
#LIBS += -lrt
#SOURCES +=  CSystrace.cpp
HEADERS +=  CSystrace.h \
            CTraceMessages.h

CONFIG += qpa/genericunixfontdatabase

LIBS += -lX11-xcb

CONFIG += link_pkgconfig
PKGCONFIG_PRIVATE += gdk-3.0 gtk+-3.0 libnotify

# for GL
PKGCONFIG_PRIVATE += egl

CONFIG += no_keywords

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QGtkIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
