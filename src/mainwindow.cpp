#include <QDebug>
#include <QThread>
#include <QDir>
#include <QScrollBar>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFontDatabase>
#include <QRegExp>
#include <QTime>
#include <QMessageBox>

#include "nfc/nfc.h"
#include "nfc/nfc-emulation.h"

#include "freefare.h"

#include "nfc/mac.h"
#include "nfc/llcp.h"
#include "nfc/llc_link.h"
#include "nfc/llc_service.h"
#include "nfc/llc_connection.h"

#include "ndef/libndef_global.h"
#include "ndef/ndefmessage.h"
#include "ndef/ndefrecord.h"
#include "ndef/ndefrecordtype.h"
#include "ndef/tlv.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pn532_extend_cmd.h"
#include "mf1ics50writeblock.h"

void sleep(unsigned int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), snepClient(new snepClientThread), snepServer(new snepServerThread)
{
    ui->setupUi(this);
    wbDialog = new mf1ics50WriteBlock(this);
    this->setWindowTitle("GNFC - " GNFC_VERSION);
    init();
}

MainWindow::~MainWindow()
{
    if(pnd != NULL)
        nfc_close(pnd);

    if(context != NULL)
        nfc_exit(context);

    pnd = NULL;
    context = NULL;

    delete wbDialog;

    delete snepClient;
    delete snepServer;

    delete ui;
}

void MainWindow :: about(void)
{
    QMessageBox::about (this, "About", "GNFC " GNFC_VERSION "\nUsing Qt "\
                        QT_VERSION_STR "\nwww.elechouse.com          ");
}

QByteArray MainWindow :: hexStr2ByteArr(QString &hexStr)
{
    QByteArray byteArr;
    if(hexStr.size()%2 != 0){
        byteArr.append((char)QString(hexStr.at(0)).toInt(0, 16));
        hexStr.remove(0, 1);
    }
    while(hexStr.size()){
        QString hex;
        hex.append(hexStr.at(0));
        hex.append(hexStr.at(1));
        byteArr.append((char)hex.toInt(0, 16));
        hexStr.remove(0, 2);
    }

    return byteArr;
}

void MainWindow::refresh()
{
    /** clear serial device list */
    ui->comboBoxSerialDevice->clear();

#ifdef Q_OS_WIN32
    int cnt = QSerialPortInfo::availablePorts().size();
    if(cnt <= 0){
        sysLog("No serial port is available.");
        return;
    }
    sysLog(QString("Find %1 Serial Port(s):").arg(cnt));
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        qDebug() << "Name        : " << info.portName();
        qDebug() << "Description : " << info.description();
        qDebug() << "Manufacturer: " << info.manufacturer();
        qDebug() << "SystemLocation: " << info.systemLocation();
        sysLog(info.portName() + "\t----\t" + info.manufacturer() + ' ' + info.description());
        ui->comboBoxSerialDevice->addItem(info.portName());
    }

#else
    QDir serialDir("/dev/serial/by-id");
    QFileInfoList serialFileList = serialDir.entryInfoList(QDir::Readable | QDir::Writable | \
                                                           QDir::NoDotAndDotDot | QDir::Files);

    for(int i=0; i<serialFileList.size(); i++){
        qDebug() << serialFileList.at(i).readLink();
        qDebug() << serialFileList.at(i).absoluteFilePath();
        if(serialFileList.at(i).isSymLink()){
            qDebug() << serialFileList.at(i).symLinkTarget();
            ui->comboBoxSerialDevice->addItem(serialFileList.at(i).symLinkTarget());
        }
    }
#endif
}

void MainWindow::beep(void)
{
    if(ui->checkBoxBeep->isChecked()){
        if(ui->rbUART->isChecked()){
            pn532ExCmd->beep(pnd);
        }else if(ui->rbNET->isChecked()){
            /* fix me: net device beep function */
        }
    }
    sleep(1);
}

void MainWindow::setOutputTextFont(void)
{

    QFontDatabase fdb;
    QStringList fl = fdb.families();
    QString fn;
    if(fl.contains("Monospace")){
        fn = "Monospace";
    }

    if(fl.contains("Consolas")){
        fn = "Consolas";
    }
    if(!fn.isEmpty()){
        qDebug() << "Set Font: " << fn;
        ui->outputText->setFontFamily(fn);
        ui->outputText->setFontPointSize(9);
        //ui->outputText->setFontPointSize();
    }
}

void MainWindow::init(void)
{
    pnd = NULL;
    context = NULL;

    sysLog("Elechouse USB NFC Demo.");

    /** get device list */
    refresh();

    /** About dialog */
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));

    /** App Quit */
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));

    /** refresh device list */
    connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));

    /** clear software log */
    connect(ui->debugInfoClearButton, SIGNAL(clicked()), ui->debugInfoText, SLOT(clear()));

    /** clear output text */
    connect(ui->outputTextClearButton, SIGNAL(clicked()), ui->outputText, SLOT(clear()));

    /** read id */
    connect(ui->readIDButton, SIGNAL(clicked()), this, SLOT(readIdInit()));

    /** open or close device */
    connect(ui->openCloseButton, SIGNAL(clicked()), this, SLOT(openClose()));

    /** reset old tag record */
    connect(ui->checkBoxVerbos, SIGNAL(toggled(bool)), this, SLOT(clearOldTag()));
    connect(ui->hexRB, SIGNAL(toggled(bool)), this, SLOT(clearOldTag()));
    //connect(ui->hexRB, SIGNAL(toggled(bool)), this, SLOT(clearOldTag()));

    connect(ui->blockRB, SIGNAL(toggled(bool)), this, SLOT(readBlockPrepare()));
    connect(ui->sectorRB, SIGNAL(toggled(bool)), this, SLOT(readBlockPrepare()));

    /** Read Blocks */
    connect(ui->readBlockButton, SIGNAL(clicked()), this, SLOT(readBlock()));

    /** password configure */
//    connect(ui->enableSetPassWordButton, SIGNAL(toggled(bool)), this, SLOT(passwdSetConfig()));
    connect(ui->keyARB, SIGNAL(toggled(bool)), this, SLOT(passwdSelect()));
    connect(ui->keyBRB, SIGNAL(toggled(bool)), this, SLOT(passwdSelect()));
    connect(ui->keyHideCheckBox, SIGNAL(toggled(bool)), this, SLOT(passwdHide()));
//    connect(ui->setPassWordButton, SIGNAL(clicked()), this, SLOT(passwdSet()));

    /** Write Block */
    connect(ui->writeBlockStartButton, SIGNAL(clicked()), this, SLOT(writeBlock()));
    connect(wbDialog, SIGNAL(sendRead(sectorData_t*)), this, SLOT(wbRead(sectorData_t*)));
    connect(wbDialog, SIGNAL(sendWrite(sectorData_t*)), this, SLOT(wbWrite(sectorData_t*)));

    /** Erase all blocks */
    connect(ui->resetBlockButton, SIGNAL(clicked()), this, SLOT(resetBlock()));

    /** test */
//    connect(ui->readBlockButton, SIGNAL(clicked()), this, SLOT(test()));
//    ui->checkBoxBeep->setStyleSheet("color: rgb(255, 0, 0);");

    /** Text Window is read-only  */
    ui->outputText->setReadOnly(true);
    ui->debugInfoText->setReadOnly(true);

    /** debugInfoText table size */
//    QFontMetrics metrics(ui->debugInfoText->font());
//    ui->debugInfoText->setTabStopWidth(6*metrics.width(' '));

    /** disable control panel when device is closed */
    ui->tabWidget->setEnabled(false);

    ui->outputText->setLineWrapMode(QTextEdit::NoWrap);

    setOutputTextFont();
    calcOutputSeperatorLength();

    passwdInit();

    /** NDEF */
    connect(ui->ndefPush, SIGNAL(clicked()), this, SLOT(ndefPush()));
    connect(snepClient, SIGNAL(finished()), this, SLOT(ndefPushed()));
    connect(ui->ndefPull, SIGNAL(clicked()), this, SLOT(ndefPull()));
    connect(snepServer, SIGNAL(finished()), this, SLOT(ndefPulled()));
    connect(snepServer, SIGNAL(sendNdefMessage(QString)), this, SLOT(ndefTextPulled(QString)));

    /** device select */
    ui->lineEditIP->setEnabled(false);
    ui->lineEditPort->setEnabled(false);
    connect(ui->rbNET, SIGNAL(toggled(bool)), this, SLOT(deviceSelect()));

    ui->tabWidget->removeTab(2);
}

void MainWindow :: deviceSelect(void)
{
    if(ui->rbNET->isChecked()){
        ui->lineEditIP->setEnabled(true);
        ui->lineEditPort->setEnabled(true);
        ui->comboBoxSerialDevice->setEnabled(false);
        ui->refreshButton->setEnabled(false);
    }else if (ui->rbUART->isChecked()){
        ui->lineEditIP->setEnabled(false);
        ui->lineEditPort->setEnabled(false);
        ui->comboBoxSerialDevice->setEnabled(true);
        ui->refreshButton->setEnabled(true);
    }
}

void MainWindow::openClose(void)
{
    ui->openCloseButton->setEnabled(false);
    ui->rbNET->setEnabled(false);
    ui->rbUART->setEnabled(false);

    if(ui->openCloseButton->text() == "Close"){

        if(pnd != NULL)
            nfc_close(pnd);

        if(context != NULL)
            nfc_exit(context);
        pnd = NULL, context = NULL;

        if(ui->rbUART->isChecked()){
            /** print log info, enable uart device */
            sysLog(ui->comboBoxSerialDevice->currentText() + " closed.");
            ui->refreshButton->setEnabled(true);
            ui->comboBoxSerialDevice->setEnabled(true);
        }else if(ui->rbNET->isChecked()){
            ui->lineEditIP->setEnabled(true);
            ui->lineEditPort->setEnabled(true);
        }

        ui->rbNET->setEnabled(true);
        ui->rbUART->setEnabled(true);
        ui->openCloseButton->setText("Open");
        ui->openCloseButton->setEnabled(true);
        ui->tabWidget->setEnabled(false);
        return;
    }

    QString dn;
    if(ui->rbUART->isChecked()){
        if(ui->comboBoxSerialDevice->count()<=0){
            sysLog("ERROR: No device found.");
            ui->openCloseButton->setEnabled(true);
            return;
        }
        if(ui->comboBoxSerialDevice->currentText().isEmpty()){
            sysLog("ERROR: Device name error");
            ui->openCloseButton->setEnabled(true);
            return;
        }

        /** choose uart device */
        dn = "pn532_uart:";
        dn += ui->comboBoxSerialDevice->currentText();
        qDebug() << dn;
    }else if(ui->rbNET->isChecked()){
        /* fix me: check text format */
        dn = "pn532_net:";
        dn += ui->lineEditIP->text() + ":" + ui->lineEditPort->text();
    }

    nfc_init(&context);
    if (context == NULL) {
        sysLog("ERROR: Unable to init libnfc");
        ui->openCloseButton->setEnabled(true);
        return;
    }

    pnd = nfc_open(context, dn.toLocal8Bit().data());
    if (pnd == NULL) {
        sysLog("ERROR: Unable to open NFC device.");
        ui->openCloseButton->setEnabled(true);
        return;
    }

    if(ui->rbUART->isChecked()){
        /** print log info, diable uart widgets */
        sysLog(ui->comboBoxSerialDevice->currentText() + " opened.");
        ui->refreshButton->setEnabled(false);
        ui->comboBoxSerialDevice->setEnabled(false);
    }else if(ui->rbNET->isChecked()){
        ui->lineEditIP->setEnabled(true);
        ui->lineEditPort->setEnabled(true);
    }

    ui->openCloseButton->setText("Close");
    ui->openCloseButton->setEnabled(true);
    ui->tabWidget->setEnabled(true);
}


void MainWindow :: test()
{
//    QString f = ui->idFormat->checkedButton()->objectName();
//    if( f == "decRB"){
//        sysLog("DEC checked.");
//    }else if(f == "hexRB"){
//        sysLog("HEX checked.");
//    }
//    outputSeperator();

//    qDebug() << "Text size" << ui->keyALineEdit->text().size();
//    QString keyString = ui->keyALineEdit->text();
//    qDebug() << "Key String Size: " << keyString.size();
//    QByteArray key;
//    if(keyString.size()%2 != 0){
//        key.append((char)QString(keyString.at(0)).toInt(0, 16));
//        keyString.remove(0, 1);
//    }
//    while(keyString.size()){
//        QString hex;
//        hex.append(keyString.at(0));
//        hex.append(keyString.at(1));
//        qDebug() << hex;
//        key.append((char)hex.toInt(0, 16));
//        keyString.remove(0, 2);
//    }

//    qDebug() << QString(key.toHex().toUpper());
//    qDebug() << (int)key.at(0);

    QString keyString = ui->keyALineEdit->text();
    QByteArray key = hexStr2ByteArr(keyString);
    qDebug() << QString(key.toHex().toUpper());
    qDebug() << (int)key.at(0);

    qDebug() << "Index: " << ui->sectorComboBox->currentIndex();
}

void MainWindow :: ndefPush(void)
{
    ui->ndefPush->setEnabled(false);
    QString str=ui->pushURL->text();
    if(!snepClient->setNdefMessage(str)){
        sysLog("URL is invalid.");
        ui->ndefPush->setEnabled(true);
        return;
    }
    snepClient->init(&pnd);
    snepClient->start();
}

void MainWindow :: ndefPushed(void)
{
    qDebug() << "ndefpush set enabled.";
    ui->ndefPush->setEnabled(true);
}

void MainWindow :: ndefPull()
{
    ui->ndefPull->setEnabled(false);
    if(snepServer->init(&pnd)<0){
        sysLog("NDEF PULL init failed.");
        return;
    }
    snepServer->start();
}

void MainWindow :: ndefPulled()
{
    qDebug() << "ndefpull set enabled.";
    ui->ndefPull->setEnabled(true);
}

void MainWindow :: ndefTextPulled(QString str)
{
    qDebug() << "Text is received.";
    if(str.isEmpty()){
        return;
    }
    ui->pullText->setText(str);
}

void MainWindow :: sysLog(QString str)
{
    int val = ui->debugInfoText->verticalScrollBar()->value();

    if( ui->debugInfoText->verticalScrollBar()->value() != ui->debugInfoText->verticalScrollBar()->maximum() ){
        ui->debugInfoText->append(str);
        ui->debugInfoText->verticalScrollBar()->setValue(val);
        return;
    }
    ui->debugInfoText->append(str);
    int maxVal = ui->debugInfoText->verticalScrollBar()->maximum();
    ui->debugInfoText->verticalScrollBar()->setValue(maxVal);
}

void MainWindow::readBlockPrepare()
{
    if(ui->blockRB->isChecked()){
        ui->block0CheckBox->setEnabled(true);
        ui->block1CheckBox->setEnabled(true);
        ui->block2CheckBox->setEnabled(true);
        ui->block3CheckBox->setEnabled(true);
    }else if(ui->sectorRB->isChecked()){
        ui->block0CheckBox->setEnabled(false);
        ui->block1CheckBox->setEnabled(false);
        ui->block2CheckBox->setEnabled(false);
        ui->block3CheckBox->setEnabled(false);
    }
}

void MainWindow::readBlock()
{
    MifareTag *tags = NULL;
    MifareClassicKey key={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    MifareClassicBlock data;
    MifareClassicKeyType keyType = MFC_KEY_A;

    int block = 0;
    int sector = 0;

    if(ui->keyARB->isChecked()){
        if(ui->keyALineEdit->text().size() != 12){
            sysLog("KEYA is too short, must be 12 characters.");
            return;
        }
        keyA = ui->keyALineEdit->text();
        QByteArray keyByteArr = hexStr2ByteArr(keyA).data();
        memcpy(key, keyByteArr.data(), keyByteArr.size());
        keyType = MFC_KEY_A;
    }else if(ui->keyBRB->isChecked()){
        if(ui->keyBLineEdit->text().size() != 12){
            sysLog("KEYB is too short, must be 12 characters.");
            return;
        }
        keyB = ui->keyBLineEdit->text();
        QByteArray keyByteArr = hexStr2ByteArr(keyA).data();
        memcpy(key, keyByteArr.data(), keyByteArr.size());
        keyType = MFC_KEY_B;
    }

    sector = ui->sectorComboBox->currentIndex();
    sector--;
    if(ui->sectorRB->isChecked()){
        block |= 0x0F;
    }else{
        if(ui->block0CheckBox->isChecked()){
            block |= 0x01;
        }
        if(ui->block1CheckBox->isChecked()){
            block |= 0x02;
        }
        if(ui->block2CheckBox->isChecked()){
            block |= 0x04;
        }
        if(ui->block3CheckBox->isChecked()){
            block |= 0x08;
        }
        if(block == 0){
            sysLog("Select one block first.");
            return;
        }
    }
    qDebug() << "block: " << block;
    qDebug() << "sector: " << sector;
    tags = freefare_get_tags(pnd);

    if(!tags){
        sysLog("No card is in field.");
        return;
    }
    int i;
    for(i=0; tags[i]; i++){
        if(freefare_get_tag_type(tags[i]) == CLASSIC_1K){
            qDebug() << "Classic 1K card found.";

            if( 0 == mifare_classic_connect(tags[i]) ){
                if(sector == -1){
                    //read all sectors
                    for(int secNum=0; secNum<16; secNum++){
                        if(0 == mifare_classic_authenticate(tags[i], (unsigned char)secNum*4, key, keyType) ){
                            for(int blockNum=0; blockNum<4; blockNum++){
                                if((block & (1<<blockNum) )== 0 ){
                                    continue;
                                }
                                if( 0 == mifare_classic_read(tags[i], (unsigned char)(secNum*4+blockNum), &data) ){
                                    QString pre=QString("Sector:%1\tBlock:%2\t").arg(secNum).arg(blockNum);
                                    ui->outputText->append(pre+QString(QByteArray((char *)data, 16).toHex().toUpper()));
                                }
                            }
                            beep();
                        }else{
                            qDebug() << "Sector: " << secNum << "authenticate failed.";
                        }
                    }
                }else{
                    if(0 == mifare_classic_authenticate(tags[i], sector*4, key, keyType) ){
                        for(int blockNum=0; blockNum<4; blockNum++){
                            if((block & (1<<blockNum) )== 0 ){
                                continue;
                            }
                            if( 0 == mifare_classic_read(tags[i], (unsigned char)(sector*4+blockNum), &data) ){
                                QString pre=QString("Sector:%1\tBlock:%2\t").arg(sector).arg(blockNum);
                                ui->outputText->append(pre+QString(QByteArray((char *)data, 16).toHex().toUpper()));
                            }
                        }
                        beep();
                    }
                }
                mifare_classic_disconnect(tags[i]);
            }
        }
    }
    qDebug() << "Tag number: " << i;
    if(i==0){
        sysLog("No card is in field.");
    }else{
        sysLog("Read block successfully.");
    }
    freefare_free_tags(tags);

}

/****************************************************************************/
/** Password */
void MainWindow :: passwdInit()
{
    QRegExp rx("[a-fA-F0-9]{,12}");
    ui->keyALineEdit->setValidator (new QRegExpValidator (rx, this));
    ui->keyBLineEdit->setValidator (new QRegExpValidator (rx, this));
//    ui->confirmLineEdit->setValidator (new QRegExpValidator (rx, this));
}

void MainWindow :: passwdSelect()
{
    if(ui->keyARB->isChecked()){
        ui->keyALineEdit->setEnabled(true);
        ui->keyBLineEdit->setEnabled(false);
    }else if (ui->keyBRB->isChecked()){
        ui->keyBLineEdit->setEnabled(true);
        ui->keyALineEdit->setEnabled(false);
    }
}

void MainWindow :: passwdHide()
{
    if(ui->keyHideCheckBox->isChecked()){
        ui->keyALineEdit->setEchoMode(QLineEdit::Password);
        ui->keyBLineEdit->setEchoMode(QLineEdit::Password);
//        ui->confirmLineEdit->setEchoMode(QLineEdit::Password);
    }else{
        ui->keyALineEdit->setEchoMode(QLineEdit::Normal);
        ui->keyBLineEdit->setEchoMode(QLineEdit::Normal);
//        ui->confirmLineEdit->setEchoMode(QLineEdit::Normal);
    }

}

void MainWindow :: passwdSetConfig()
{
//    if(ui->enableSetPassWordButton->isChecked()){
//        /** clear keyA and keyB */

//        /** enable widgets */
//        ui->confirmLineEdit->setEnabled(true);
//        ui->setPassWordButton->setEnabled(true);
//    }else{
//        /** restore keyA and keyB */

//        /** disable widgets*/
//        ui->confirmLineEdit->setEnabled(false);
//        ui->setPassWordButton->setEnabled(false);
//    }

}

void MainWindow :: passwdSet()
{
    test();
}


/****************************************************************************/
/** write block */
void MainWindow :: writeBlock()
{
    wbDialog->open();
//    wbDialog->exec();
    qDebug() << "Write Block Finish.";
}

void MainWindow :: wbWrite(sectorData_t * sector)
{
    MifareTag *tags = NULL;
    MifareClassicKey key={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    MifareClassicBlock data;
    MifareClassicKeyType keyType;

    tags = freefare_get_tags(pnd);

    if(!tags){
        sysLog("No card is in field.");
        wbDialog->recieveWrite(false);
        return;
    }

    if(!tags[0]){
        sysLog("No card is in field.");
        wbDialog->recieveWrite(false);
        return;
    }

    QByteArray keyByteArr = hexStr2ByteArr(*sector->key->keyData).data();
    memcpy(key, keyByteArr.data(), keyByteArr.size());
    keyType = sector->key->type;

    if(freefare_get_tag_type(tags[0]) == CLASSIC_1K){
        sysLog("Classic 1K card found.");
        if( 0 == mifare_classic_connect(tags[0]) ){
            if(0 == mifare_classic_authenticate(tags[0], sector->sector*4+1, key, keyType) ){
                if(sector->block->blockMap&0x01){
                    QByteArray dataByteArr = hexStr2ByteArr(*sector->block->block0).data();
                    memcpy(data, dataByteArr.data(), dataByteArr.size());
                    if( 0 == mifare_classic_write(tags[0], (unsigned char)((sector->sector)*4+0), data) ){
                        QString pre=QString("Sector:%1 Block:%2").arg(sector->sector).arg(0);
                        sysLog("Write "+pre+" successfully.");
                    }else{
                        QString pre=QString("Sector:%1 Block:%2").arg(sector->sector).arg(0);
                        sysLog("Write "+pre+" failed.");
                    }
                }
                if(sector->block->blockMap&0x02){
                    QByteArray dataByteArr = hexStr2ByteArr(*sector->block->block1).data();
                    memcpy(data, dataByteArr.data(), dataByteArr.size());
                    if( 0 == mifare_classic_write(tags[0], (unsigned char)((sector->sector)*4+1), data) ){
                        QString pre=QString("Sector:%1 Block:%2").arg(sector->sector).arg(1);
                        sysLog("Write "+pre+" successfully.");
                    }else{
                        QString pre=QString("Sector:%1 Block:%2").arg(sector->sector).arg(1);
                        sysLog("Write "+pre+" failed.");
                    }
                }
                if(sector->block->blockMap&0x04){
                    QByteArray dataByteArr = hexStr2ByteArr(*sector->block->block2).data();
                    memcpy(data, dataByteArr.data(), dataByteArr.size());
                    if( 0 == mifare_classic_write(tags[0], (unsigned char)((sector->sector)*4+2), data) ){
                        QString pre=QString("Sector:%1 Block:%2").arg(sector->sector).arg(2);
                        sysLog("Write "+pre+" successfully.");
                    }else{
                        QString pre=QString("Sector:%1 Block:%2").arg(sector->sector).arg(2);
                        sysLog("Write "+pre+" failed.");
                    }
                }
                if(sector->block->blockMap&0x08){
                    QString sectorTrailor;
                    sectorTrailor += *sector->block->blockKeyA;
                    sectorTrailor += *sector->block->blockControl;
                    sectorTrailor += *sector->block->blockKeyB;
                    qDebug() << sectorTrailor;
                    QByteArray dataByteArr = hexStr2ByteArr(sectorTrailor).data();
                    memcpy(data, dataByteArr.data(), dataByteArr.size());
                    if( 0 == mifare_classic_write(tags[0], (unsigned char)((sector->sector)*4+3), data) ){
                        QString pre=QString("Sector:%1 trailer(block 3)").arg(sector->sector);
                        sysLog("Write "+pre+" successfully.");
                    }else{
                        QString pre=QString("Sector:%1 trailer(block 3)").arg(sector->sector);
                        sysLog("Write "+pre+" failed.");
                    }
                }
                beep();
            }
            mifare_classic_disconnect(tags[0]);
        }
    }else{
        sysLog("Not a Classic 1K card.");
        wbDialog->recieveWrite(false);
        return;
    }

    wbDialog->recieveWrite(true);

    qDebug() << "wbWrite";
}

void MainWindow :: wbRead(sectorData_t *sector)
{
    MifareTag *tags = NULL;
    MifareClassicKey key={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    MifareClassicBlock data;
    MifareClassicKeyType keyType;


    blockData_t *blockData = new blockData_t;
    blockData->block0 = new QString("");
    blockData->block1 = new QString("");
    blockData->block2 = new QString("");
    blockData->blockKeyA = new QString("");
    blockData->blockControl = new QString("");
    blockData->blockKeyB = new QString("");

    tags = freefare_get_tags(pnd);

    if(!tags){
        sysLog("No card is in field.");
        wbDialog->recieveRead(NULL, false);
        return;
    }

    if(!tags[0]){
        sysLog("No card is in field.");
        wbDialog->recieveRead(NULL, false);
        return;
    }

    blockData->blockMap = 0;

    QByteArray keyByteArr = hexStr2ByteArr(*sector->key->keyData).data();
    memcpy(key, keyByteArr.data(), keyByteArr.size());
    keyType = sector->key->type;

    if(freefare_get_tag_type(tags[0]) == CLASSIC_1K){
        sysLog("Classic 1K card found.");
        if( 0 == mifare_classic_connect(tags[0]) ){
            if(0 == mifare_classic_authenticate(tags[0], sector->sector*4+1, key, keyType) ){
                if(sector->block->blockMap&0x01){
                    if( 0 == mifare_classic_read(tags[0], (unsigned char)((sector->sector)*4+0), &data) ){
                        blockData->blockMap |= 0x01;
                        *blockData->block0 = QString(QByteArray((char *)data, 16).toHex().toUpper());
                    }
                }
                if(sector->block->blockMap&0x02){
                    if( 0 == mifare_classic_read(tags[0], (unsigned char)((sector->sector)*4+1), &data) ){
                        blockData->blockMap |= 0x02;
                        *blockData->block1 = QString(QByteArray((char *)data, 16).toHex().toUpper());
                    }
                }
                if(sector->block->blockMap&0x04){
                    if( 0 == mifare_classic_read(tags[0], (unsigned char)((sector->sector)*4+2), &data) ){
                        blockData->blockMap |= 0x04;
                        *blockData->block2 = QString(QByteArray((char *)data, 16).toHex().toUpper());
                    }
                }
                if(sector->block->blockMap&0x08){
                    if( 0 == mifare_classic_read(tags[0], (unsigned char)((sector->sector)*4+3), &data) ){
                        blockData->blockMap |= 0x08;
                        QString block3 = QString(QByteArray((char *)data, 16).toHex().toUpper());
                        qDebug() << block3;
                        /** KeyA is not readable,  */
                        *blockData->blockKeyA = "FFFFFFFFFFFF";
                        block3.remove(0, 12);
                        *blockData->blockControl = block3.left(8);
                        block3.remove(0, 8);
                        *blockData->blockKeyB = block3.left(12);
                    }
                }
                if( 0 != blockData->blockMap){
                    qDebug() << "receiveRead";
                    wbDialog->recieveRead(blockData, true);
                }
                beep();
            }
            mifare_classic_disconnect(tags[0]);
        }
    }else{
        sysLog("Not a Classic 1K card.");
        wbDialog->recieveRead(NULL, false);
        return;
    }

    qDebug() << "wbRead";
}

/****************************************************************************/
/** reset all blocks */
void MainWindow :: resetBlock()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Mifare Card Erase", "Are you sure to erase all blocks?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    qDebug() << "Erase all blocks.";

    MifareTag *tags = NULL;
    MifareClassicKey key={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    MifareClassicBlock data;
    MifareClassicBlock block3={
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x07, 0x80, 0x69,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    MifareClassicKeyType keyType = MFC_KEY_A;

    memset(data, 0, sizeof(MifareClassicBlock));

    int block = 0;
    int sector = 0;

    if(ui->keyARB->isChecked()){
        if(ui->keyALineEdit->text().size() != 12){
            sysLog("KEYA is too short, must be 12 characters.");
            return;
        }
        keyA = ui->keyALineEdit->text();
        QByteArray keyByteArr = hexStr2ByteArr(keyA).data();
        memcpy(key, keyByteArr.data(), keyByteArr.size());
        keyType = MFC_KEY_A;
    }else if(ui->keyBRB->isChecked()){
        if(ui->keyBLineEdit->text().size() != 12){
            sysLog("KEYB is too short, must be 12 characters.");
            return;
        }
        keyB = ui->keyBLineEdit->text();
        QByteArray keyByteArr = hexStr2ByteArr(keyA).data();
        memcpy(key, keyByteArr.data(), keyByteArr.size());
        keyType = MFC_KEY_B;
    }

    ui->openCloseButton->setEnabled(false);
    ui->readIDButton->setEnabled(false);
    ui->readBlockButton->setEnabled(false);
    ui->writeBlockStartButton->setEnabled(false);

    tags = freefare_get_tags(pnd);

    if(!tags){
        sysLog("No card is in field.");
        ui->openCloseButton->setEnabled(true);
        ui->readIDButton->setEnabled(true);
        ui->readBlockButton->setEnabled(true);
        ui->writeBlockStartButton->setEnabled(true);
        return;
    }

    if(!tags[0]){
        sysLog("No card is in field.");
        ui->openCloseButton->setEnabled(true);
        ui->readIDButton->setEnabled(true);
        ui->readBlockButton->setEnabled(true);
        ui->writeBlockStartButton->setEnabled(true);
        return;
    }

    if(freefare_get_tag_type(tags[0]) == CLASSIC_1K){
        if( 0 == mifare_classic_connect(tags[0]) ){
            for(sector=0; sector<16; sector++){
                for(block=0; block<4; block++){
                    if(0 == mifare_classic_authenticate(tags[0], sector*4+block, key, keyType) ){
                        if(sector==0 && block==0){
                            continue;
                        }
                        if(block != 3){
                            if( 0 == mifare_classic_write(tags[0], (unsigned char)(sector*4+block), data) ){
                                QString pre=QString("Sector:%1 Block:%2").arg(sector).arg(block);
                                sysLog("Reset "+pre+" successfully.");
                            }else{
                                QString pre=QString("Sector:%1 Block:%2").arg(sector).arg(block);
                                sysLog("Reset "+pre+" failed.");
                            }
                        }else{
                            if( 0 == mifare_classic_write(tags[0], (unsigned char)(sector*4+block), block3) ){
                                QString pre=QString("Sector:%1 Block:%2").arg(sector).arg(block);
                                sysLog("Reset "+pre+" successfully.");
                            }else{
                                QString pre=QString("Sector:%1 Block:%2").arg(sector).arg(block);
                                sysLog("Reset "+pre+" failed.");
                            }
                        }
                    }
                }
                beep();
            }
            beep();
            mifare_classic_disconnect(tags[0]);
        }
    }else{
        sysLog("Not a Classic 1K card.");
    }

    ui->openCloseButton->setEnabled(true);
    ui->readIDButton->setEnabled(true);
    ui->readBlockButton->setEnabled(true);
    ui->writeBlockStartButton->setEnabled(true);

    QMessageBox::information(this, "Mifare Card Erase", "Factroy Erase Done." );
}

/****************************************************************************/
void MainWindow :: readIdInit()
{
    if(pnd==NULL || context == NULL){
        sysLog("Device is not opened");
        return;
    }

    if(ui->readIDButton->text() == "Stop"){
        sysLog("Stop read ID.");

        /** Enable buttons */
        ui->openCloseButton->setEnabled(true);
        ui->readBlockButton->setEnabled(true);
        ui->writeBlockStartButton->setEnabled(true);
        ui->resetBlockButton->setEnabled(true);

        t->stop();
        disconnect(this, SLOT(readId()));
        delete t;
        ui->readIDButton->setText("ReadID");
    }else{
        sysLog("Start read ID.");

        /** Enable buttons */
        ui->openCloseButton->setEnabled(false);
        ui->readBlockButton->setEnabled(false);
        ui->writeBlockStartButton->setEnabled(false);
        ui->resetBlockButton->setEnabled(false);

        t = new QTimer(this);
        connect(t, SIGNAL(timeout()), this, SLOT(readId()));
        t->start(1000);
        ui->readIDButton->setText("Stop");
    }
}

QString MainWindow::checkBaudRate()
{
    QString br;
    switch(nt.nm.nbr){
    case NBR_UNDEFINED:
    default:
        br = "NA";
        break;
    case NBR_106:
        br = "106k";
        break;
    case NBR_212:
        br = "212k";
        break;
    case NBR_424:
        br = "424k";
        break;
    case NBR_847:
        br = "847k";
        break;
    }
    return br;
}

QString MainWindow::checkIdType()
{
    switch(nt.nm.nmt){
    case NMT_ISO14443A:
        return "14443A";
        break;
    case NMT_ISO14443B:
        return "14443-4B";
        break;
    case NMT_ISO14443BI:
        return "14443-4B'";
        break;
    case NMT_ISO14443B2CT:
        return "14443-2BCT";
        break;
    case NMT_ISO14443B2SR:
        return "14443-2BSR";
        break;
    case NMT_FELICA:
        return "FeliCa";
        break;
    case NMT_JEWEL:
        return "Innovision Jewel";
        break;
    case NMT_DEP:
        return "D.E.P.";
        break;
    }
    return "";
}

QByteArray MainWindow::checkId()
{
    QByteArray res;
    switch(nt.nm.nmt){
    case NMT_ISO14443A:
        res.append((char *)nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
        break;
    case NMT_ISO14443B:
        res.append((char *)nt.nti.nbi.abtPupi, 4);
        break;
    case NMT_ISO14443BI:
        res.append((char *)nt.nti.nii.abtDIV, 4);
        break;
    case NMT_ISO14443B2CT:
        res.append((char *)nt.nti.nci.abtUID, 4);
        break;
    case NMT_ISO14443B2SR:
        res.append((char *)nt.nti.nsi.abtUID, 8);
        break;
    case NMT_FELICA:
        res.append((char *)nt.nti.nfi.abtId, 8);
        break;
    case NMT_JEWEL:
        res.append((char *)nt.nti.nji.btId, 4);
        break;
    case NMT_DEP:
        res.append((char *)nt.nti.ndi.abtNFCID3, 10);
        break;
    }
    return res;
}

bool MainWindow::isNewTag()
{
    newTag = checkId();

    if(oldTag != newTag){
        oldTag = newTag;
        return true;
    }

    return false;
}

void MainWindow::clearOldTag(void)
{
    QRadioButton *cb = dynamic_cast<QRadioButton *>(sender());
    if(cb){
        if(ui->checkBoxVerbos->isChecked()){
            return;
        }
    }
    oldTag.clear();
}

void MainWindow::readId()
{
    if(pnd==NULL || context == NULL){
        sysLog("Device is not opened");

        return;
    }
    const nfc_modulation nmMods[8] = {
        {NMT_ISO14443A, NBR_106},
        {NMT_FELICA, NBR_212},
        {NMT_FELICA, NBR_424},
        {NMT_ISO14443B, NBR_106},
        {NMT_ISO14443BI, NBR_106},
        {NMT_ISO14443B2SR, NBR_106},
        {NMT_ISO14443B2CT, NBR_106},
        {NMT_JEWEL, NBR_106},
    };
    int res;

    // Set opened NFC device to initiator mode
    if (nfc_initiator_init(pnd) < 0) {
        nfc_perror(pnd, "nfc_initiator_init");
        sysLog("ERROR: nfc_initiator_init");
        return;
    }
    for(int nmCnt=0; nmCnt<8; nmCnt++){
        if ((res = nfc_initiator_list_passive_targets(pnd, nmMods[nmCnt], &nt, 1)) < 0) {
            nfc_perror(pnd, "nfc_initiator_poll_target");
            qDebug() << "Poll Target Timeout.";
            oldTag.clear();
            continue;
        }

        if(res>0){
            if(!isNewTag()){
                beep();
                return ;
            }

            qDebug() <<"Target Found.";
            if(ui->checkBoxVerbos->isChecked()){
                char *s;
                str_nfc_target(&s, &nt, true);
                ui->outputText->append(s);
                nfc_free(s);
            }else{
                QString f = ui->idFormat->checkedButton()->objectName();
                if( f == "decRB"){
                    bool ok;
                    ui->outputText->append(QString::number(QString(newTag.toHex().toUpper()).toULongLong(&ok, 16)));
                    qDebug() << QString(newTag.toHex().toUpper());
                    qDebug() << QString(newTag.toHex().toUpper()).toULongLong(&ok, 16);
                }else if(f == "hexRB"){
                    ui->outputText->append(QString(newTag.toHex().toUpper()));
                }
            }
            beep();
        }else{
            if(!oldTag.isEmpty()){
                oldTag.clear();
            }
            //sysLog("No target found.");
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    return;


    if(pnd==NULL || context == NULL){
        sysLog("Device is not opened");

        return;
    }

    const nfc_modulation nmModulations[5] = {
        { NMT_ISO14443A, NBR_106 },
        { NMT_ISO14443B, NBR_106 },
        { NMT_FELICA,  NBR_212 },
        { NMT_FELICA, NBR_424 },
        { NMT_JEWEL, NBR_106 },
    };

    const uint8_t uiPollNr = 1;
    const uint8_t uiPeriod = 1;
    const size_t szModulations = 5;


    // Set opened NFC device to initiator mode
    if (nfc_initiator_init(pnd) < 0) {
        nfc_perror(pnd, "nfc_initiator_init");
        sysLog("ERROR: nfc_initiator_init");
        return;
    }

    if ((res = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt))  < 0) {
        nfc_perror(pnd, "nfc_initiator_poll_target");
        sysLog("Poll Target Timeout.");
        oldTag.clear();
        return;
    }

    if(res>0){
        beep();
        if(!isNewTag()){
            return ;
        }

        sysLog("Target Found.");
        if(ui->checkBoxVerbos->isChecked()){
            char *s;
            str_nfc_target(&s, &nt, true);
            ui->outputText->append(s);
            nfc_free(s);
        }else{
            QString f = ui->idFormat->checkedButton()->objectName();
            if( f == "decRB"){
                bool ok;
                ui->outputText->append(QString::number(QString(newTag.toHex().toUpper()).toULongLong(&ok, 16)));
                qDebug() << QString(newTag.toHex().toUpper());
                qDebug() << QString(newTag.toHex().toUpper()).toULongLong(&ok, 16);
            }else if(f == "hexRB"){
                ui->outputText->append(QString(newTag.toHex().toUpper()));
            }
        }
    }else{
        sysLog("No target found.");
    }
}


/*****************************************************************************/
/** Output seperator */
void MainWindow::outputSeperator(void)
{
    QString sep(outputSeperatorLength, '-');
    ui->outputText->append(sep);
}

void MainWindow::calcOutputSeperatorLength(void)
{
    qDebug() << "Calculate Sep length";
    QString sep;
    QFont ft(ui->outputText->fontFamily(), ui->outputText->fontPointSize());
    qDebug() << "Font name:" << ft.family() << "size: " << ft.pointSize();
    QFontMetrics fm(ft);
    int width = ui->outputText->width() - ui->outputText->contentsMargins().right() \
                        -ui->outputText->contentsMargins().left();
    for(; ;){
        sep.append('-');
        if(fm.width(sep)>=width){
            if(ui->outputText->verticalScrollBar()->isVisible()){
                sep.remove(0, 3);

            }else{
                sep.remove(0, 6);
            }
            break;
        }
    }
    outputSeperatorLength = sep.length();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    calcOutputSeperatorLength();
    QWidget::resizeEvent(event);
}

