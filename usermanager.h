#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QDialog>

const QString api = "http://127.0.0.1:5000/";

namespace Ui {
class UserManager;
}

class UserManager : public QDialog
{
    Q_OBJECT

public:
    explicit UserManager(QWidget *parent = nullptr);
    ~UserManager();
    QString username;
    bool loginState;

signals:
    void sendUsername(QString username);

private slots:
    void on_loginButton_clicked();

    void on_loginCancelButton_clicked();

    void on_registerButton_clicked();

    void on_registerCancelButton_clicked();

private:
    Ui::UserManager *ui;
};

#endif // USERMANAGER_H
