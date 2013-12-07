#include "mf1ics50writeblock.h"
#include "ui_mf1ics50writeblock.h"

#include <QMessageBox>
#include <QDebug>
#include <QRegExp>
#include <QFont>
#include <QFontDatabase>

mf1ics50WriteBlock::mf1ics50WriteBlock(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mf1ics50WriteBlock)
{
    ui->setupUi(this);

    /** key part*/
    connect(ui->radioButtonKeyA, SIGNAL(toggled(bool)), this, SLOT(keySelect()));
    connect(ui->checkBoxKeyHide, SIGNAL(toggled(bool)), this, SLOT(keyHide()));
    ui->lineEditKeyB->setEnabled(false);
    ui->checkBoxKeyHide->setChecked(true);

    /** block */
    connect(ui->checkBoxBlock0, SIGNAL(toggled(bool)), this, SLOT(blockCheck()));
    connect(ui->checkBoxBlock1, SIGNAL(toggled(bool)), this, SLOT(blockCheck()));
    connect(ui->checkBoxBlock2, SIGNAL(toggled(bool)), this, SLOT(blockCheck()));
    connect(ui->checkBoxBlock3, SIGNAL(toggled(bool)), this, SLOT(blockCheck()));
    ui->checkBoxBlock0->setChecked(true);
    ui->checkBoxBlock1->setChecked(true);
    ui->checkBoxBlock2->setChecked(true);
    ui->checkBoxBlock3->setChecked(false);

    /** write/read */
    connect(ui->pushButtonReadBlock, SIGNAL(clicked()), this, SLOT(read()));
    connect(ui->pushButtonWriteBlock, SIGNAL(clicked()), this, SLOT(write()));

    /** Limit Line Edit Input(Regular Expression) */
    QRegExp rx("[a-fA-F0-9]{,8}");
    ui->lineEditBlock3Control->setValidator (new QRegExpValidator (rx, this));

    rx = QRegExp("[a-fA-F0-9]{,12}");
    ui->lineEditBlock3KeyA->setValidator (new QRegExpValidator (rx, this));
    ui->lineEditBlock3KeyB->setValidator (new QRegExpValidator (rx, this));
    ui->lineEditKeyA->setValidator (new QRegExpValidator (rx, this));
    ui->lineEditKeyB->setValidator (new QRegExpValidator (rx, this));

    rx = QRegExp("[a-fA-F0-9]{,32}");
    ui->lineEditBlock0->setValidator (new QRegExpValidator (rx, this));
    ui->lineEditBlock1->setValidator (new QRegExpValidator (rx, this));
    ui->lineEditBlock2->setValidator (new QRegExpValidator (rx, this));

    /** change default font of lineEdit */
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
        QFont f(fn);
        f.setPointSize(10);
        ui->lineEditBlock0->setFont(f);
        ui->lineEditBlock1->setFont(f);
        ui->lineEditBlock2->setFont(f);
        //ui->outputText->setFontPointSize();
    }

    writeSector = new sectorData_t;
    writeSector->block = new blockData_t;
    writeSector->block->block0 = new QString("");
    writeSector->block->block1 = new QString("");
    writeSector->block->block2 = new QString("");
    writeSector->block->blockControl = new QString("");
    writeSector->block->blockKeyA = new QString("");
    writeSector->block->blockKeyB = new QString("");
    writeSector->key = new keyData_t;
    writeSector->key->keyData = new QString("");

    readSector = new sectorData_t;
    readSector->block = new blockData_t;
    readSector->block->block0 = new QString("");
    readSector->block->block1 = new QString("");
    readSector->block->block2 = new QString("");
    readSector->block->blockControl = new QString("");
    readSector->block->blockKeyA = new QString("");
    readSector->block->blockKeyB = new QString("");
    readSector->key = new keyData_t;
    readSector->key->keyData = new QString("");


    qDebug() << "mf1ics50 init";
}

mf1ics50WriteBlock::~mf1ics50WriteBlock()
{
    qDebug() << "mf1ics50 delete";
    delete writeSector;
    delete readSector;
    delete ui;
}

void mf1ics50WriteBlock::keyHide()
{
    if(ui->checkBoxKeyHide->isChecked()){
        ui->lineEditKeyA->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        ui->lineEditKeyB->setEchoMode(QLineEdit::Password);
    }else{
        ui->lineEditKeyA->setEchoMode(QLineEdit::Normal);
        ui->lineEditKeyB->setEchoMode(QLineEdit::Normal);
    }
}

void mf1ics50WriteBlock :: keySelect()
{
    if(ui->radioButtonKeyA->isChecked()){
        ui->lineEditKeyA->setEnabled(true);
        ui->lineEditKeyB->setEnabled(false);
    }else if(ui->radioButtonKeyB->isChecked()){
        ui->lineEditKeyB->setEnabled(true);
        ui->lineEditKeyA->setEnabled(false);
    }
}

void mf1ics50WriteBlock :: blockCheck()
{
    if(ui->checkBoxBlock0->isChecked()){
        ui->lineEditBlock0->setEnabled(true);
    }else{
        ui->lineEditBlock0->setEnabled(false);
    }
    if(ui->checkBoxBlock1->isChecked()){
        ui->lineEditBlock1->setEnabled(true);
    }else{
        ui->lineEditBlock1->setEnabled(false);
    }
    if(ui->checkBoxBlock2->isChecked()){
        ui->lineEditBlock2->setEnabled(true);
    }else{
        ui->lineEditBlock2->setEnabled(false);
    }
    if(ui->checkBoxBlock3->isChecked()){
        ui->lineEditBlock3Control->setEnabled(true);
        ui->lineEditBlock3KeyA->setEnabled(true);
        ui->lineEditBlock3KeyB->setEnabled(true);
        ui->lineEditBlock3KeyA->setStyleSheet("color: rgb(255, 0, 0)");
        ui->lineEditBlock3KeyB->setStyleSheet("color: rgb(255, 0, 0)");
        ui->lineEditBlock3Control->setStyleSheet("color: rgb(255, 0, 0)");
    }else{
        ui->lineEditBlock3Control->setEnabled(false);
        ui->lineEditBlock3KeyA->setEnabled(false);
        ui->lineEditBlock3KeyB->setEnabled(false);
        ui->lineEditBlock3KeyA->setStyleSheet("");
        ui->lineEditBlock3KeyB->setStyleSheet("");
        ui->lineEditBlock3Control->setStyleSheet("");
    }
}

bool mf1ics50WriteBlock :: isKeyValid()
{
    /** Check key length */
    int keyLen;
    if(ui->radioButtonKeyA->isChecked()){
        keyLen = ui->lineEditKeyA->text().size();
    }else if(ui->radioButtonKeyB->isChecked()){
        keyLen = ui->lineEditKeyB->text().size();
    }

    if(keyLen != 12){
        QMessageBox::warning(this, tr("Error"),
                             tr("Key length is too short.\n"
                                "Must be 12 characters."),
                             QMessageBox::Ok);
        return false;
    }

    /** Block Check */
    if( !(ui->checkBoxBlock0->isChecked() || ui->checkBoxBlock1->isChecked() \
            || ui->checkBoxBlock2->isChecked() || ui->checkBoxBlock3->isChecked())){
        QMessageBox::warning(this, tr("Error"),
                             tr("No block is selected\n"),
                             QMessageBox::Ok);
        return false;
    }

    return true;
}

uint8_t mf1ics50WriteBlock :: isWriteBlockValid()
{
    int blockMap = 0;

    if(ui->checkBoxBlock0->isChecked()){
        blockMap |= 0x01;
         if(ui->lineEditBlock0->text().size() != 32){
             QMessageBox::warning(this, tr("Error"),
                                  tr("Block 0 data must be 32 characters"),
                                  QMessageBox::Ok);
             return 0;
         }
    }
    if(ui->checkBoxBlock1->isChecked()){
        blockMap |= 0x02;
        if(ui->lineEditBlock1->text().size() != 32){
            QMessageBox::warning(this, tr("Error"),
                                 tr("Block 1 data must be 32 characters"),
                                 QMessageBox::Ok);
            return 0;
        }
    }
    if(ui->checkBoxBlock2->isChecked()){
        blockMap |= 0x04;
        if(ui->lineEditBlock2->text().size() != 32){
            QMessageBox::warning(this, tr("Error"),
                                 tr("Block 2 data must be 32 characters"),
                                 QMessageBox::Ok);
            return 0;
        }
    }
    if(ui->checkBoxBlock3->isChecked()){
        blockMap |= 0x08;
        if(ui->lineEditBlock3Control->text().size() != 8){
            QMessageBox::warning(this, tr("Error"),
                                 tr("Block3 control area must contain 8 characters"),
                                 QMessageBox::Ok);
            return 0;
        }
        if(ui->lineEditBlock3KeyB->text().size() != 12){
            QMessageBox::warning(this, tr("Error"),
                                 tr("Block3 KeyA area must contain 12 characters"),
                                 QMessageBox::Ok);
            return 0;
        }
        if(ui->lineEditBlock3KeyA->text().size() != 12){
            QMessageBox::warning(this, tr("Error"),
                                 tr("Block3 KeyB area must contain 12 characters"),
                                 QMessageBox::Ok);
            return 0;
        }

        /** check access bits format */
        uint32_t ab = ui->lineEditBlock3Control->text().toUInt(NULL, 16);
        qDebug() << QString::number(ab, 16);
        uint8_t cxx[6];
        ab >>= 8;
        for(int i=0; i<6; i++){
            cxx[5-i] = (uint8_t)(ab & 0x0F);
            ab >>= 4;
        }
        if( (0x0F != cxx[0]+cxx[5]) || (0x0F != cxx[1]+cxx[2]) || (0x0F != cxx[3]+cxx[4]) ){

            QMessageBox::warning(this, tr("Error"),
                                 tr("Access bits are in wrong format."),
                                 QMessageBox::Ok);
            return 0;
        }
    }

    if(blockMap == 0){
        QMessageBox::warning(this, tr("Error"),
                             tr("No block is selected."),
                             QMessageBox::Ok);
        return 0;
    }

    return blockMap;
}

uint8_t mf1ics50WriteBlock :: isReadBlockValid()
{
    int blockMap = 0;
    if(ui->checkBoxBlock0->isChecked()){
        blockMap |= 0x01;
    }
    if(ui->checkBoxBlock1->isChecked()){
        blockMap |= 0x02;
    }
    if(ui->checkBoxBlock2->isChecked()){
        blockMap |= 0x04;
    }
    if(ui->checkBoxBlock3->isChecked()){
        blockMap |= 0x08;
    }

    if(blockMap == 0){
        QMessageBox::warning(this, tr("Error"),
                             tr("No block is selected."),
                             QMessageBox::Ok);
    }

    return blockMap;
}

void mf1ics50WriteBlock :: read()
{
    if(!isKeyValid()){
        return;
    }
    uint8_t bm = isReadBlockValid();
    if(bm == 0){
        return;
    }

    readSector->sector = ui->comboBoxSector->currentIndex();
    readSector->block->blockMap = bm;
    if(ui->radioButtonKeyA->isChecked()){
        readSector->key->type = MFC_KEY_A;
        *readSector->key->keyData = ui->lineEditKeyA->text();
    }else{
        readSector->key->type = MFC_KEY_B;
        *readSector->key->keyData = ui->lineEditKeyB->text();
    }

    emit sendRead(readSector);

    qDebug() << "Read block(s) succeessfully.";
}

void mf1ics50WriteBlock :: write()
{
    uint8_t bm;
    if(!isKeyValid()){
        return;
    }
    bm = isWriteBlockValid();
    if(bm == 0){
        return;
    }

    writeSector->sector = ui->comboBoxSector->currentIndex();
    writeSector->block->blockMap = bm;
    *writeSector->block->block0 = ui->lineEditBlock0->text();
    *writeSector->block->block1 = ui->lineEditBlock1->text();
    *writeSector->block->block2 = ui->lineEditBlock2->text();
    *writeSector->block->blockKeyA = ui->lineEditBlock3KeyA->text();
    *writeSector->block->blockKeyB = ui->lineEditBlock3KeyB->text();
    *writeSector->block->blockControl = ui->lineEditBlock3Control->text();
    if(ui->radioButtonKeyA->isChecked()){
        writeSector->key->type = MFC_KEY_A;
        *writeSector->key->keyData = ui->lineEditKeyA->text();
    }else{
        writeSector->key->type = MFC_KEY_B;
        *writeSector->key->keyData = ui->lineEditKeyB->text();
    }


    emit sendWrite(writeSector);

    qDebug() << "Write blocks succeessfully.";
}

void mf1ics50WriteBlock :: recieveRead(blockData_t *blockData, bool ret)
{
    if(!ret){
        QMessageBox::warning(this, tr("Error"),
                             tr("Read block failed"),
                             QMessageBox::Ok);
        return ;
    }

    if(blockData == NULL){
        QMessageBox::warning(this, tr("Error"),
                             tr("Read block failed"),
                             QMessageBox::Ok);
        return ;
    }

    if(blockData->blockMap&0x01 && blockData->block0 != NULL){
        ui->lineEditBlock0->setText(*blockData->block0);
    }
    if(blockData->blockMap&0x02 && blockData->block1 != NULL){
        ui->lineEditBlock1->setText(*blockData->block1);
    }
    if(blockData->blockMap&0x04 && blockData->block2 != NULL){
        ui->lineEditBlock2->setText(*blockData->block2);
    }
    if(blockData->blockMap&0x08){
        if(blockData->blockKeyA != NULL){
            ui->lineEditBlock3KeyA->setText(*blockData->blockKeyA);
        }
        if(blockData->blockKeyB != NULL){
            ui->lineEditBlock3KeyB->setText(*blockData->blockKeyB);
        }
        if(blockData->blockControl != NULL){
            ui->lineEditBlock3Control->setText(*blockData->blockControl);
        }
    }
}

void mf1ics50WriteBlock :: recieveWrite(bool sta)
{
    if(!sta){
        QMessageBox::warning(this, tr("Error"),
                             tr("Write block failed"),
                             QMessageBox::Ok);
    }else{
        QMessageBox::information(this, tr("Error"),
                             tr("Write block successed"),
                             QMessageBox::Ok);
    }
}

void mf1ics50WriteBlock :: test()
{
    recieveWrite(true);
    recieveWrite(false);
}
