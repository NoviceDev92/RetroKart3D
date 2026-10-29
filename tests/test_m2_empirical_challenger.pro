QT += core gui widgets testlib

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = test_m2_empirical_challenger
TEMPLATE = app

INCLUDEPATH += .. .

SOURCES += \
    test_m2_empirical_challenger.cpp \
    ../game_canvas.cpp \
    ../sprite_atlas.cpp

HEADERS += \
    ../game_canvas.h \
    ../sprite_atlas.h
