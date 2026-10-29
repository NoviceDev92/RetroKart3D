QT += core gui widgets testlib

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = standalone_m3_forensic
TEMPLATE = app

INCLUDEPATH += .. .

SOURCES += \
    standalone_m3_forensic.cpp \
    ../game_canvas.cpp \
    ../sprite_atlas.cpp

HEADERS += \
    ../game_canvas.h \
    ../sprite_atlas.h
