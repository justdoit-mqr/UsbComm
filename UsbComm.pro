#-------------------------------------------------
#
# Project created by QtCreator 2021-03-15T13:30:45
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UsbComm
TEMPLATE = app

#头文件包含路径    方便代码中直接包含子目录下的头文件
INCLUDEPATH += 3rdparty/

SOURCES += main.cpp\
        widget.cpp \
    usbcomm.cpp

HEADERS  += widget.h \
    usbcomm.h

FORMS    += widget.ui

LIBS += -L./3rdparty/libusb-1.0/lib -lusb-1.0
