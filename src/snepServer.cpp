#include "QDebug"
#include "QDataStream"
#include "snepServer.h"

struct mac_link* snepServerThread :: mac_link;
QString snepServerThread :: pl;
snepServerThread * snepServerThread :: ptr;

snepServerThread :: snepServerThread(nfc_device **device)
{
    pnd = NULL;
    mac_link = NULL;
    llc_link = NULL;
    flag = false;
    device = device;

    connect(this, SIGNAL(finished()), this, SLOT(free()));
    ptr = this;
}

snepServerThread :: ~snepServerThread()
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

void snepServerThread :: free()
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

int snepServerThread :: init(nfc_device **device)
{
    pnd = *device;

    if (llcp_init() < 0){
        qDebug() << "llcp_init() failed.";
        return -1;
    }

    llc_link = llc_link_new();
    if (!llc_link) {
        qDebug() << "Cannot allocate LLC link data structures";
        return -1;
    }

    /** initial LLCP service struct */
    struct llc_service *com_android_snep;
    if (!(com_android_snep = llc_service_new_with_uri(NULL, snepService, (char *)"urn:nfc:sn:snep", NULL))){
        qDebug() << "Cannot create com.android.snep service";
    }
    llc_service_set_miu(com_android_snep, 512);
    llc_service_set_rw(com_android_snep, 2);

    if (llc_link_service_bind(llc_link, com_android_snep, LLCP_SNEP_SAP) < 0){
        qDebug() << "Cannot bind service";
    }
    mac_link = mac_link_new(pnd, llc_link);
    if (!mac_link){
        qDebug() << "Cannot create MAC link";
        return -1;
    }

    qDebug() << "snep: flag set true";
    flag = true;

    return 0;
}

void snepServerThread :: stop()
{
    if(mac_link != NULL)
    mac_link_deactivate(mac_link, MAC_DEACTIVATE_ON_REQUEST);
}

void snepServerThread :: run()
{
    /** wait  */
    while(!flag);
    qDebug() << "snepClientThread running.";

    if(mac_link == NULL || llc_link == NULL || con == NULL){
        qDebug() << "Device is not initialed.";
        return;
    }

    if (mac_link_activate_as_target(mac_link) < 0) {
        qDebug() << "Cannot activate MAC link";
        return;
    }

    void *err;
    mac_link_wait(mac_link, &err);
}

void *snepServerThread ::snepService(void *arg)
{
    qDebug() << "snepService";
    struct llc_connection *connection = (struct llc_connection *) arg;
    uint8_t buffer[1024], frame[1024];

    int len;
    if ((len = llc_connection_recv(connection, buffer, sizeof(buffer), NULL)) < 0)
        return NULL;

    if (len < 2) // SNEP's header (2 bytes)  and NDEF entry header (5 bytes)
        return NULL;

    size_t n = 0;

    // Header
    printf("SNEP version: %d.%d\n", (buffer[0]>>4), (buffer[0]&0x0F));
    if (buffer[n++] != 0x10){
        printf("snep-server is developed to support snep version 1.0, version %d.%d may be not supported.\n", (buffer[0]>>4), (buffer[0]&0x0F));
    }
    switch(buffer[1]){
    case 0x00:      /** Continue */
        break;
    case 0x01:      /** GET */
        break;
    case 0x02:      /** PUT */
    {
        uint32_t ndef_length = be32toh(*((uint32_t *)(buffer + 2)));  // NDEF length
        if (((uint32_t)len - 6) < ndef_length)
            return NULL; // Less received bytes than expected ?

        /** return snep success response package */
        frame[0] = 0x10;    /** SNEP version */
        frame[1] = 0x81;
        frame[2] = 0;
        frame[3] = 0;
        frame[4] = 0;
        frame[5] = 0;
        llc_connection_send(connection, frame, 6);

        QByteArray ba;
        ba.append((char *)buffer+6, ndef_length);
        pl.clear();
        decodeNDEFMessage(ba);
        ptr->sendNdefMessage(pl);
    }
        break;
    }

    if ((len = llc_connection_recv(connection, buffer, sizeof(buffer), NULL)) < 0){
        printf("Receiv nothing.\n");
        mac_link_deactivate(mac_link, MAC_DEACTIVATE_ON_REQUEST);
        return NULL;
    }
    printf("len: %d\n", len);

    qDebug() << "sencond recv quit.";

    // TODO Stop the LLCP when this is reached
    llc_connection_stop(connection);\

    return NULL;
}

QString snepServerThread :: toTypeNameFormat (int id)
{
    switch(id)
    {
        case NDEFRecordType::NDEF_Empty: 	return QString("Empty"); break;
        case NDEFRecordType::NDEF_NfcForumRTD: 	return QString("NFC Forum well-known type"); break;
        case NDEFRecordType::NDEF_MIME: 	return QString("Media-type as defined in RFC 2046"); break;
        case NDEFRecordType::NDEF_URI: 		return QString("Absolute URI as defined in RFC 3986"); break;
        case NDEFRecordType::NDEF_ExternalRTD: 	return QString("NFC Forum external type"); break;
        case NDEFRecordType::NDEF_Unknown: 	return QString("Unknown"); break;
        case NDEFRecordType::NDEF_Unchanged: 	return QString("Unchanged"); break;
        case 7: 				return QString("Reserved"); break;
    }
    return QString("Invalid");
}

void snepServerThread :: decodeNDEFMessage (const QByteArray data, int depth)
{
    NDEFMessage msg = NDEFMessage::fromByteArray (data);
    QString prefix("");
    for (int d=0; d<depth; d++) prefix.append("    ");

    if (msg.isValid()) {
        qDebug() << prefix << "NDEF message is valid and contains " << msg.recordCount() << " NDEF record(s)." << endl;
        int i = 0;
        foreach (const NDEFRecord& record, msg.records())
        {
            i++;
            qDebug() << prefix << "NDEF record (" << i << ") type name format: " << toTypeNameFormat(record.type().id()) << endl;
            const QString type_name = record.type().name();
            qDebug() << prefix << "NDEF record (" << i << ") type: " << type_name << endl;

            switch (record.type().id())
            {
                case NDEFRecordType::NDEF_NfcForumRTD:
                    if (type_name == QString("Sp"))
                    {
                        decodeNDEFMessage (record.payload(), ++depth);
                    }
                    else if (type_name == QString("T"))
                    {
                        const QString locale_string = NDEFRecord::textLocale(record.payload()).replace('-', '_');
                        // const QString locale_string = NDEFRecord::textLocale(record.payload());
                        // const QString locale_string = record.payload().toHex();
                        QLocale locale(locale_string);
                        qDebug() << prefix << "NDEF record (" << i << ") payload (language): " << QLocale::languageToString (locale.language()) << " (" << locale_string << ")" << endl;
                        qDebug() << prefix << "NDEF record (" << i << ") payload (text): " << NDEFRecord::textText(record.payload()) << endl;
                        pl.append("TEXT: "+NDEFRecord::textText(record.payload()));
                    }
                    else if (type_name == QString("U"))
                    {
                        qDebug() << prefix << "NDEF record (" << i << ") payload (uri): " << NDEFRecord::uriProtocol (record.payload()) << record.payload().mid(1) << endl;
                        pl.append(NDEFRecord::uriProtocol (record.payload()) + record.payload().mid(1));
                    }
                    else if ((depth > 0) && (type_name == QString("act")))
                    {
                        quint8 action = record.payload().at(0);
                        qDebug() << prefix << "NDEF record (" << i << ") payload (action code): " << action << endl;
                    }
                    else if ((depth > 0) && (type_name == QString("s")))
                    {
                        QDataStream stream(record.payload());
                        qint32 size;
                        stream >> size;
                        qDebug() << prefix << "NDEF record (" << i << ") payload (size): " << size << endl;
                    }
                    else if ((depth > 0) && (type_name == QString("t")))
                    {
                        qDebug() << prefix << "NDEF record (" << i << ") payload (type): " << QString::fromUtf8(record.payload()) << endl;
                    }
                    else
                    {
                        qDebug() << prefix << "NDEF record (" << i << ") payload (hex): " << record.payload().toHex() << endl;
                    }
                break;
                case NDEFRecordType::NDEF_MIME:
                default:
                    qDebug() << prefix << "NDEF record (" << i << ") payload (hex): " << record.payload().toHex() << endl;
            }
        }
    }
    else
    {
        qDebug() << "Invalid NDEF message." << endl;
    }
}
