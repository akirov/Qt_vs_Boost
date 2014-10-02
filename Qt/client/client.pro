QT       += core
QT       += network
QT       -= gui

TARGET   = client

CONFIG   += console
CONFIG   -= app_bundle

CONFIG   += debug
CONFIG   -= release

DESTDIR = ../exe

TEMPLATE = app

SOURCES += main.cpp \
           Client.cpp

HEADERS += Client.hpp
