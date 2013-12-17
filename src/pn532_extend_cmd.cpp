#include <Qt>

#include "pn532_extend_cmd.h"
#include <nfc/nfc.h>
#include <stdio.h>
#include <unistd.h>

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

struct nfc_device_dual {
  const nfc_context *context;
  const struct nfc_driver *driver;
  void *driver_data;
  void *chip_data;

  /** Device name string, including device wrapper firmware */
  char    name[256];
  /** Device connection string */
  nfc_connstring connstring;
  /** Is the CRC automaticly added, checked and removed from the frames */
  bool    bCrc;
  /** Does the chip handle parity bits, all parities are handled as data */
  bool    bPar;
  /** Should the chip handle frames encapsulation and chaining */
  bool    bEasyFraming;
  /** Should the chip switch automatically activate ISO14443-4 when
      selecting tags supporting it? */
  bool    bAutoIso14443_4;
  /** Supported modulation encoded in a byte */
  uint8_t  btSupportByte;
  /** Last reported error */
  int     last_error;
};

#ifdef Q_OS_WIN32
struct serial_port_windows {
  HANDLE  hPort;                // Serial port handle
  DCB     dcb;                  // Device control settings
  COMMTIMEOUTS ct;              // Serial port time-out configuration
};
#define UART_DATA( X ) ((struct serial_port_windows *) (X))
#else
 struct serial_port_unix {
   int 			fd; 			// Serial port file descriptor
   struct termios 	termios_backup; 	// Terminal info before using the port
   struct termios 	termios_new; 		// Terminal info during the transaction
 };
 #define UART_DATA( X ) ((struct serial_port_unix *) (X))
#endif /** Q_OS_WIN32 */

typedef void *serial_port;
// Internal data structs
struct pn532_uart_data {
  serial_port port;
#ifndef WIN32
  int     iAbortFds[2];
#else
  volatile bool abort_flag;
#endif
};

#define DRIVER_DATA(pnd) ((struct pn532_uart_data*)((pnd)->driver_data))

pn532_extend_cmd::pn532_extend_cmd()
{


}

int pn532_extend_cmd::sendCommand(nfc_device *pnd, uint8_t *buf, uint8_t len)
{
    if(pnd==NULL){
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

#ifdef WIN32
    DWORD   dwTxLen = 0;
    int res = WriteFile((UART_DATA((DRIVER_DATA(((nfc_device_dual *)pnd))->port)))->hPort, frame, len+8, &dwTxLen, NULL);
#else
     int res = write( (UART_DATA((DRIVER_DATA(((nfc_device_dual *)pnd))->port)))->fd, frame, len+8);
#endif

    delete [] frame;

    if(res != len+8){
        return -1;
    }

#ifdef WIN32
    if (!dwTxLen)
        return -1;
#endif

    return 0;
}

int pn532_extend_cmd::beep(nfc_device *pnd)
{
    uint8_t *buf = new uint8_t[1];

    buf[0] = PN532_EX_CMD_BEEP;

    sendCommand(pnd, buf, 1);

    delete [] buf;

    return 0;
}
