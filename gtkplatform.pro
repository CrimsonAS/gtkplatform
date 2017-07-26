TARGET = qgtk

QT += core-private gui-private platformsupport-private

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            qgtkintegration.cpp \
            qgtkbackingstore.cpp
HEADERS =   qgtkintegration.h \
            qgtkbackingstore.h

CONFIG += qpa/genericunixfontdatabase

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QGtkIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
