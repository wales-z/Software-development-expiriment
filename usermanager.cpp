#include "usermanager.h"
#include "ui_usermanager.h"
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>


UserManager::UserManager(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserManager)
{
    ui->setupUi(this);
    this->loginState = false;
    this->username="";
    ui->loginPassword->setEchoMode(QLineEdit::Password);
    ui->registerPassword->setEchoMode(QLineEdit::Password);
}

UserManager::~UserManager()
{
    delete ui;
}

void UserManager::on_loginButton_clicked()
{
    if(ui->loginUsername->text()=="") {
        QMessageBox::information(this, "提示", "用户名不能为空。",QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if(ui->loginPassword->text()=="") {
        QMessageBox::information(this, "提示", "密码不能为空。",QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(api + "login/"+ui->loginUsername->text()+"/"+ui->loginPassword->text()));
    QNetworkReply* reply = manager->get(request);

    connect(manager, &QNetworkAccessManager::finished, this, [=] (QNetworkReply* _reply) {
        if (_reply->readAll().toStdString() == "success") {
            QMessageBox::information(this, "提示", "登录成功。",QMessageBox::Yes, QMessageBox::Yes);
            this->username = ui->loginUsername->text();
            this->loginState = true;
            this->close();
        } else {
            QMessageBox::information(this, "提示", "登录失败，请检查用户名/密码是否输入正确，或网络连接状况。",QMessageBox::Yes, QMessageBox::Yes);
            this->loginState = false;
        }
    });
}


void UserManager::on_loginCancelButton_clicked()
{
    this->close();
}


void UserManager::on_registerButton_clicked()
{
    if(ui->registerUsername->text()=="") {
        QMessageBox::information(this, "提示", "用户名不能为空。",QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if(ui->registerPassword->text()=="") {
        QMessageBox::information(this, "提示", "密码不能为空。",QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(api + "register/"+ui->registerUsername->text()+"/"+ui->registerPassword->text()));
    QNetworkReply* reply = manager->get(request);

    connect(manager, &QNetworkAccessManager::finished, this, [=] (QNetworkReply* _reply) {
        if (_reply->readAll().toStdString() == "success") {
            QMessageBox::information(this, "提示", "注册成功。",QMessageBox::Yes, QMessageBox::Yes);
            this->username = ui->loginUsername->text();
            this->loginState = true;
            emit this->sendUsername(this->username);
        } else {
            QMessageBox::information(this, "提示", "注册失败，请检查网络连接状况。",QMessageBox::Yes, QMessageBox::Yes);
            this->loginState = false;
        }
    });
}


void UserManager::on_registerCancelButton_clicked()
{
    this->close();
}

