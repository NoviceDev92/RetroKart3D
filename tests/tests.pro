QT += core gui widgets

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = test_runner
TEMPLATE = app

INCLUDEPATH += .. .

SOURCES += \
    main.cpp \
    test_harness.cpp \
    test_tier1_features.cpp \
    test_tier2_boundaries.cpp \
    test_tier3_interactions.cpp \
    test_tier4_realworld.cpp \
    ../sprite_atlas.cpp

HEADERS += \
    test_types.h \
    test_framework.h \
    test_harness.h \
    test_tier1_features.h \
    test_tier2_boundaries.h \
    test_tier3_interactions.h \
    test_tier4_realworld.h \
    ../sprite_atlas.h
