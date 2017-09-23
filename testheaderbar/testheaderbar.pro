QT += widgets gui-private

TEMPLATE = app
TARGET = testheaderbar
INCLUDEPATH += ../src

# Input
SOURCES += main.cpp

CONFIG += link_pkgconfig
PKGCONFIG += gtk+-3.0

CONFIG += no_keywords
