#ifndef SNEPCLIENT_H
#define SNEPCLIENT_H

/** QT Headers */
#include <QThread>

/** libnfc headers */
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

/** libllcp headers */
#include <nfc/llcp.h>
#include <nfc/mac.h>
#include <nfc/llc_link.h>
#include <nfc/llc_connection.h>
#include <nfc/llc_service.h>

/** libndef headers */
#include <ndef/ndefmessage.h>
#include <ndef/ndefrecord.h>
#include <ndef/tlv.h>

class snepClientThread : public QThread{
    Q_OBJECT

public:
    snepClientThread(nfc_device **device=NULL);
    ~snepClientThread();
    int init(nfc_device **device);
    bool setNdefMessage(QString str);
signals:


protected:
    void run();
private slots:
    void free();
private:
    static void *snepService(void *arg);

    nfc_device *pnd;

    struct mac_link *mac_link;
    struct llc_link *llc_link;
    struct llc_connection *con;

    bool flag;

    static NDEFMessage msg;
    static QByteArray package;
};

#endif // SNEPCLIENT_H
