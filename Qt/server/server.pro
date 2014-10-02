QT       += core
QT       += network
QT       -= gui

TARGET   = server

CONFIG   += console
CONFIG   -= app_bundle

CONFIG   += debug
CONFIG   -= release

DESTDIR = ../exe

TEMPLATE = app

SOURCES += main.cpp \
           Server.cpp

HEADERS += Server.hpp
