#include "backupconfigdialog.h"
#include "ui_backupconfigdialog.h"
#include <QFileDialog>
#include <QMessageBox>


BackupConfigDialog::BackupConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BackupConfigDialog)
{
    ui->setupUi(this);
    ui->passwordLineEdit_3->setEchoMode(QLineEdit::Password);
    this->backupFilename = nullptr;
    this->backupFileDirectory = nullptr;
    this->noneRegular = nullptr;
    this->everyday = nullptr;
    this->everyweek = nullptr;
    this->passwordCheckBox = nullptr;
    this->password = nullptr;
    this->cloudBackup = nullptr;

}

BackupConfigDialog::~BackupConfigDialog()
{
    delete ui;
}



void BackupConfigDialog::on_browseButton_2_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("备份到"),
        "/home",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory != "") {
        ui->backupFileDirectoryLineEdit->setText(directory);
    }
}


void BackupConfigDialog::on_passwordCheckBox_3_stateChanged(int arg1)
{
    ui->passwordLineEdit_3->setEnabled(ui->passwordCheckBox_3->checkState());
}


void BackupConfigDialog::on_startButton_clicked()
{
//    emit this->backupConfig(
//        ui->backupFilenameLineEdit,
//        ui->backupFileDirectoryLineEdit,
//        ui->noneRadioButton,
//        ui->everydayRadioButton_2,
//        ui->everyweekRadioButton_3,
//        ui->passwordCheckBox_3,
//        ui->passwordLineEdit_3,
//        ui->cloudCheckBox_2);
    this->backupFilename = ui->backupFilenameLineEdit;
    this->backupFileDirectory = ui->backupFileDirectoryLineEdit;
    this->noneRegular = ui->noneRadioButton;
    this->everyday = ui->everydayRadioButton_2;
    this->everyweek = ui->everyweekRadioButton_3;
    this->passwordCheckBox = ui->passwordCheckBox_3;
    this->password = ui->passwordLineEdit_3;
    this->cloudBackup = ui->cloudCheckBox_2;

    this->close();
}

void BackupConfigDialog::on_cancelButton_clicked()
{
    this->close();
    this->~BackupConfigDialog();
}

