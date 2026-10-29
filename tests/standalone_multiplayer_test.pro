QT += core network

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = test_multiplayer
TEMPLATE = app

INCLUDEPATH += .. .

SOURCES += \
    standalone_multiplayer_test.cpp \
    ../network_manager.cpp

HEADERS += \
    test_framework.h \
    ../network_manager.h
