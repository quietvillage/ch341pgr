QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

QMAKE_LFLAGS += -no-pie

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ch341interface.cpp \
    chipdata.cpp \
    croppingdialog.cpp \
    hexview.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ch341interface.h \
    chipdata.h \
    croppingdialog.h \
    hexview.h \
    mainwindow.h

FORMS += \
    croppingdialog.ui \
    mainwindow.ui

TRANSLATIONS += \
    ch341pgr_en.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

unix: LIBS += -L$$PWD/libusb/ -lusb-1.0

INCLUDEPATH += $$PWD/libusb
DEPENDPATH += $$PWD/libusb
