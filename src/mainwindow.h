#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGlobal>
#include <QMainWindow>
#include <QTimer>
#include <nfc/nfc.h>
#include "pn532_extend_cmd.h"
#include "mf1ics50writeblock.h"

#include "snepClient.h"
#include "snepServer.h"

#define GNFC_VERSION        "0.2.0"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QByteArray hexStr2ByteArr(QString &hexStr);

signals:
    void deviceFound();

private slots:
    void refresh();
    void readIdInit();
    void readId();
    void openClose();
    void test();
    bool isNewTag();
    void clearOldTag();

    void readBlockPrepare();
    void readBlock();

    void passwdSelect(void);
    void passwdSetConfig(void);
    void passwdSet(void);
    void passwdHide(void);

    void writeBlock();
    void wbRead(sectorData_t * sector);
    void wbWrite(sectorData_t * sector);

    /** clear all data  */
    void resetBlock();

    /** About */
    void about(void);

    /** ndef */
    void ndefPush(void);
    void ndefPushed(void);
    void ndefPull(void);
    void ndefPulled(void);
    void ndefTextPulled(QString str);

    /** uart or net device choose */
    void deviceSelect(void);

protected:
    void resizeEvent ( QResizeEvent * event );

private:
    void sysLog(QString str);
    void beep(void);
    void init(void);
    void calcOutputSeperatorLength(void);
    void outputSeperator(void);

    void setOutputTextFont(void);

    void passwdInit(void);

    QString checkBaudRate(void);
    QString checkIdType(void);
    QByteArray checkId(void);

    Ui::MainWindow *ui;
    mf1ics50WriteBlock *wbDialog;

    /**NFC*/
    nfc_device *pnd;
    nfc_target nt;
    nfc_context *context;

    pn532_extend_cmd *pn532ExCmd;

    QTimer *t;

    QByteArray oldTag, newTag;
    QString keyA, keyB, keyConfirm;

    int outputSeperatorLength;

    snepClientThread *snepClient;
    snepServerThread *snepServer;
};

#endif // MAINWINDOW_H
