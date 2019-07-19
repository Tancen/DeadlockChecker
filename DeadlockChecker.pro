QT += core
QT -= gui

CONFIG += c++11

TARGET = DeadlockChecker
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += ENABLE_DEADLOCK_CHECK

SOURCES += test.cpp \
    ./src/DeadlockChecker.cpp \
    ReadWriteLock.cpp 

HEADERS += \
    ./src/DeadlockChecker.h \
    ReadWriteLock.h \
