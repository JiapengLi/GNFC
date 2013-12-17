QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += serialport

TARGET = nfc-gui
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    pn532_extend_cmd.cpp \
    mf1ics50writeblock.cpp 

!win32 {
    SOURCES += snepClient.cpp \
    snepServer.cpp
}

HEADERS  += mainwindow.h \
    pn532_extend_cmd.h \
    mf1ics50writeblock.h 
!win32 {
    HEADERS += snepClient.h \
               snepServer.h
}

FORMS    += mainwindow.ui \
    mf1ics50writeblock.ui

!win32{
    LIBS += -lnfc -lfreefare -lnfc-llcp -lndef
}

win32 {
    INCLUDEPATH += "C:\libnfc\include" "C:\libfreefare\include\freefare"
    LIBS += -L"C:\libnfc\lib" -L"C:\libfreefare\lib"
    LIBS += -lnfc -lfreefare
}

#Version Control
#GIT_VERSION = $$system($$quote(git describe))
#GIT_TIMESTAMP = $$system($$quote(git log -n 1 --format=format:"%at"))
#QMAKE_SUBSTITUTES += $$PWD/version.h.in
