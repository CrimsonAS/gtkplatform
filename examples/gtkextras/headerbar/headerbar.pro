QT += widgets gui-private gtkextras

TEMPLATE = app
TARGET = testheaderbar
INCLUDEPATH += ../src

# Input
SOURCES += main.cpp

CONFIG += link_pkgconfig
PKGCONFIG += gtk+-3.0

CONFIG += no_keywords

target.path = $$[QT_INSTALL_EXAMPLES]/gtkextras/headerbar
INSTALLS += target
