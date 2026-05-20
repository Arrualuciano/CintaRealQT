QT += core gui widgets serialport
CONFIG += c++17
TARGET = CintaHMI
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    serialmanager.cpp

HEADERS += \
    mainwindow.h \
    serialmanager.h \
    protocol.h
