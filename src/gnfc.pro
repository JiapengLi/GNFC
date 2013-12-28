QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += serialport

TARGET = gnfc
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    pn532_extend_cmd.cpp \
    mf1ics50writeblock.cpp 
SOURCES += snepClient.cpp \
    snepServer.cpp


HEADERS  += mainwindow.h \
    pn532_extend_cmd.h \
    mf1ics50writeblock.h 
HEADERS += snepClient.h \
    snepServer.h


FORMS    += mainwindow.ui \
    mf1ics50writeblock.ui

!win32{
    LIBS += -lnfc -lfreefare -lllcp -lndef
}

win32 {
    INCLUDEPATH += "C:\libnfc\include" "C:\libfreefare\include\freefare" "C:\libndef\include"
    LIBS += -L"C:\libnfc\lib" -L"C:\libfreefare\lib" -L"C:\libndef\lib"
    LIBS += -lnfc -lfreefare -lllcp -lndef1 -lws2_32
}

