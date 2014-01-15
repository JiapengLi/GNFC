#ifndef EWIN_BEEP_H
#define EWIN_BEEP_H

#include <QTcpSocket>
#include <stdint.h>
#include <stdio.h>

class ewin_beep : public QTcpSocket
{
public:
    ewin_beep();
    int beep(void);
    int sendCommand(uint8_t *buf, int len);
public slots:

};

#endif // EWIN_BEEP_H
