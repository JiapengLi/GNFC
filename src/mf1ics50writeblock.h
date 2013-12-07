#ifndef MF1ICS50WRITEBLOCK_H
#define MF1ICS50WRITEBLOCK_H

#include <QDialog>
#include <freefare.h>

typedef struct{
    uint8_t blockMap;
    QString *block0;
    QString *block1;
    QString *block2;
    QString *blockKeyA;
    QString *blockControl;
    QString *blockKeyB;
} blockData_t;

typedef struct{
    MifareClassicKeyType type;
    QString *keyData;
}keyData_t;

typedef struct{
    uint8_t sector;
    blockData_t *block;
    keyData_t *key;
}sectorData_t;

namespace Ui {
class mf1ics50WriteBlock;
}

class mf1ics50WriteBlock : public QDialog
{
    Q_OBJECT


public:
    explicit mf1ics50WriteBlock(QWidget *parent = 0);
    ~mf1ics50WriteBlock();


public slots:
    void recieveRead(blockData_t *blockData, bool ret);
    void recieveWrite(bool sta);
signals:
    void sendRead(sectorData_t *);
    void sendWrite(sectorData_t *);

private slots:
    void keyHide(void);
    void keySelect(void);
    void blockCheck(void);

    void write(void);
    void read(void);

    void test(void);
private:
    bool isKeyValid(void);
    uint8_t isWriteBlockValid(void);
    uint8_t isReadBlockValid(void);

    Ui::mf1ics50WriteBlock *ui;
    sectorData_t *writeSector;
    sectorData_t *readSector;
};

#endif // MF1ICS50WRITEBLOCK_H
