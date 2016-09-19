#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:53
#
#-------------------------------------------------
QMAKE_CXXFLAGS += -std=c++11

QT       -= gui network

TARGET = QMailer
TEMPLATE = lib
CONFIG += staticlib

include(qmailer.pri)

unix {
    target.path = /usr/lib
    INSTALLS += target
}
