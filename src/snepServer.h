#ifndef SNEPSERVER_H
#define SNEPSERVER_H
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

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#define be32toh(x) ntohl(x)
#endif

class snepServerThread : public QThread{
    Q_OBJECT

public:
    snepServerThread(nfc_device **device=NULL);
    ~snepServerThread();
    int init(nfc_device **device);
    void stop();

signals:
    void sendNdefMessage(QString);
    void serviceFinish();

protected:
    void run();

private slots:
    void free();

private:
    static void *snepService(void *arg);
    static QString toTypeNameFormat(int id);
    static void  decodeNDEFMessage (const QByteArray data, int depth = 0);

    nfc_device *pnd;

    static struct mac_link *mac_link;
    struct llc_link *llc_link;
    struct llc_connection *con;

    bool flag;

    static QString pl;
    static snepServerThread *ptr;
};

#endif // SNEPSERVER_H
