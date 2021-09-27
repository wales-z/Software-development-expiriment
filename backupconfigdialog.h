#ifndef BACKUPCONFIGDIALOG_H
#define BACKUPCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>


namespace Ui {
class BackupConfigDialog;
}

class BackupConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackupConfigDialog(QWidget *parent = nullptr);
    ~BackupConfigDialog();
    QLineEdit* backupFilename;
    QLineEdit* backupFileDirectory;
    QRadioButton* noneRegular;
    QRadioButton* everyday;
    QRadioButton* everyweek;
    QCheckBox* passwordCheckBox;
    QLineEdit* password;
    QCheckBox* cloudBackup;

signals:
    void backupConfig(
        QLineEdit* backupFilename,
        QLineEdit* backupFileDirectory,
        QRadioButton* noneRegular,
        QRadioButton* everyday,
        QRadioButton* everyweek,
        QCheckBox* passwordCheckbox,
        QLineEdit* password,
        QCheckBox* cloudBackup);

private slots:
    void on_browseButton_2_clicked();

    void on_passwordCheckBox_3_stateChanged(int arg1);

    void on_startButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::BackupConfigDialog *ui;
};

#endif // BACKUPCONFIGDIALOG_H
