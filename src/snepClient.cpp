#include "QDebug"
#include "snepClient.h"

NDEFMessage snepClientThread :: msg=NDEFMessage(NDEFRecord::createUriRecord("http://elechouse.com"));
QByteArray snepClientThread :: package;

snepClientThread :: snepClientThread(nfc_device **device)
{
    pnd = NULL;
    mac_link = NULL;
    llc_link = NULL;
    flag = false;
    device = device;

    connect(this, SIGNAL(finished()), this, SLOT(free()));
}

snepClientThread :: ~snepClientThread()
{
    if(mac_link != NULL){
        mac_link_free(mac_link);
        mac_link = NULL;
    }
    if(llc_link != NULL){
        llc_link_free(llc_link);
        llc_link= NULL;
    }
    llcp_fini();
}

void snepClientThread :: free()
{
    qDebug() << "snepClientThread is stopped.";
    flag = false;
    if(mac_link != NULL){
        mac_link_free(mac_link);
        mac_link = NULL;
    }
    if(llc_link != NULL){
        llc_link_free(llc_link);
        llc_link= NULL;
    }
}

int snepClientThread :: init(nfc_device **device)
{
    pnd = *device;

    if (llcp_init() < 0){
        qDebug() << "llcp_init() failed.";
        return -1;
    }

    llc_link = llc_link_new();
    if (!llc_link) {
        qDebug() << "Cannot allocate LLC link data structures";
    }

    mac_link = mac_link_new(pnd, llc_link);
    if (!mac_link){
        qDebug() << "Cannot create MAC link";
        return -1;
    }

    struct llc_service *com_android_npp;
    if (!(com_android_npp = llc_service_new(NULL, snepService, NULL))){
        qDebug() << "Cannot create com.android.npp service";
        return -1;
    }
    llc_service_set_miu(com_android_npp, 512);
    llc_service_set_rw(com_android_npp, 2);

    int sap;
    if ((sap = llc_link_service_bind(llc_link, com_android_npp, 0x20)) < 0){
        qDebug() << "Cannot create com.android.npp service";
        return -1;
    }

    con = llc_outgoing_data_link_connection_new(llc_link, sap, LLCP_SNEP_SAP);
    if (!con){
        qDebug() << "Connection created failed.";
        return -1;
    }

    qDebug() << "snep: flag set true";
    flag = true;

    return 0;
}

void snepClientThread :: run()
{
    /** wait  */
    while(!flag);
    qDebug() << "snepClientThread running.";

    if(mac_link == NULL || llc_link == NULL || con == NULL){
        qDebug() << "Device is not initialed.";
        return;
    }

    if (mac_link_activate_as_initiator(mac_link) < 0) {
        qDebug() << "Cannot activate MAC link";
        return;
    }

    if (llc_connection_connect(con) < 0){
        qDebug() << "Cannot connect llc_connection";
    }
    llc_connection_wait(con, NULL);

    llc_link_deactivate(llc_link);
}

void *snepClientThread ::snepService(void *arg)
{
    qDebug() << "snepService";
    struct llc_connection *connection = (struct llc_connection *) arg;
    uint8_t buf[1024];
    int ret;
    uint8_t ssap;
#if 0
    uint8_t frame[] = {
        0x10, 0x02,
        0x00, 0x00, 0x00, 33,
        0xd1, 0x02, 0x1c, 0x53, 0x70, 0x91, 0x01, 0x09, 0x54, 0x02,
        0x65, 0x6e, 0x4c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0x51, 0x01,
        0x0b, 0x55, 0x03, 0x6c, 0x69, 0x62, 0x6e, 0x66, 0x63, 0x2e,
        0x6f, 0x72, 0x67
    };

    llc_connection_send(connection, frame, sizeof(frame));

    ret = llc_connection_recv(connection, buf, sizeof(buf), &ssap);
    if(ret>0){
        printf("Send NDEF message done.\n");
    }else if(ret ==  0){
        printf("Received no data\n");
    }else{
        printf("Error received data.");
    }

    sleep(2);
    uint8_t frame1[] = {
        0x10, 0x02,
        0x00, 0x00, 0x00, 33,
        0xD1, 0x01, 0x1D, 0x55, 0x03, 0x67, 0x69, 0x74, 0x68, 0x75,
        0x62, 0x2E, 0x63, 0x6F, 0x6D, 0x2F, 0x6A, 0x69, 0x61, 0x70,
        0x65, 0x6E, 0x67, 0x6C, 0x69, 0x2F, 0x6C, 0x69, 0x62, 0x6C,
        0x6C, 0x63, 0x70
    };

    llc_connection_send(connection, frame1, sizeof(frame1));
    ret = llc_connection_recv(connection, buf, sizeof(buf), &ssap);
    if(ret>0){
        printf("Send NDEF message done.\n");
    }else if(ret ==  0){
        printf("Received no data\n");
    }else{
        printf("Error received data.");
    }

    sleep(2);
#endif
    if(msg.isValid()){
        quint32 len = msg.toByteArray().size();
        uint8_t header[6];
        header[0] = 0x10;
        header[1] = 0x02;
        header[2] = (uint8_t)(len>>24);
        header[3] = (uint8_t)((len&0x00FFFFFF)>>16);
        header[4] = (uint8_t)((len&0x0000FFFF)>>8);
        header[5] = (uint8_t)(len&0x000000FF);
        package.clear();
        package.append((char *)header, 6);
        package.append(msg.toByteArray());

        qDebug() << QString(package.toHex().toUpper());
        llc_connection_send(connection, (uint8_t *)package.data(), package.size());
        ret = llc_connection_recv(connection, buf, sizeof(buf), &ssap);
        if(ret>0){
            printf("Send NDEF message done.\n");
        }else if(ret ==  0){
            printf("Received no data\n");
        }else{
            printf("Error received data.");
        }
    }

    llc_connection_stop(connection);

    return NULL;
}

bool snepClientThread::setNdefMessage(QString str)
{
    /** set message */

    msg.setRecord(NDEFRecord::createUriRecord(str));
    return msg.isValid();
}
