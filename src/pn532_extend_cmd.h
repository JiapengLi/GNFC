#ifndef PN532_EXTEND_CMD_H
#define PN532_EXTEND_CMD_H

#include <nfc/nfc.h>
#include <stdint.h>
#include <stdio.h>

#define PN532_EX_CMD_BEEP					(0xA0)

class pn532_extend_cmd
{
public:
    pn532_extend_cmd();
    int sendCommand(nfc_device *pnd, uint8_t *buf, uint8_t len);
    int beep(nfc_device *pnd);
};

#endif // PN532_EXTEND_CMD_H
