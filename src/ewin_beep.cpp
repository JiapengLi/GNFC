#include "ewin_beep.h"
#include <QDataStream>
#include <QtNetwork>

ewin_beep::ewin_beep()
{

}

int ewin_beep :: sendCommand(uint8_t *buf, int len)
{
    if(state() != QAbstractSocket::ConnectedState){
        return -1;
    }

    if(!len){
        return -1;
    }

    uint8_t *frame = new uint8_t[len+8];
    frame[0] = 0x00;
    frame[1] = 0x00;
    frame[2] = 0xFF;
    frame[3] = len + 1;
    frame[4] = ~(len+1) + 1;
    frame[5] = 0xD4;
    uint8_t sum = 0xD4;
    for(int i=0; i<len; i++){
        frame[6+i] = buf[i];
        sum += buf[i];
    }
    frame[len+6] = ~sum+1;
    frame[len+7] = 0;


    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    out.setVersion(QDataStream::Qt_5_1);
    out.writeRawData((const char *)frame, len+8);

    int res = write(block);

    delete [] frame;

    if(res != len+8){
        return -1;
    }

    return 0;
}

#define PN532_EX_CMD_BEEP					(0xA0)

int ewin_beep::beep(void)
{
    uint8_t *buf = new uint8_t[1];

    buf[0] = PN532_EX_CMD_BEEP;

    sendCommand(buf, 1);

    delete [] buf;

    return 0;
}

