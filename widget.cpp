#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileDialog>
#include <QListView>
#include <QMessageBox>
#include <regex>
#include <QDesktopServices>
#include <QProcess>
#include "usermanager.h"
#include "ui_usermanager.h"
#include "backupconfigdialog.h"

Widget::Widget(QWidget* parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    this->setFixedSize({768, 610});
    ui->passwordLineEdit_2->setEchoMode(QLineEdit::Password);
    ui->passwordLineEdit_3->setEchoMode(QLineEdit::Password);
    ui->mainWindowLoginButton->show();
    ui->mainWindowQuitButton->hide();
    this->currentUser = "";
    ui->userLabel->setText("未登录");
    this->isUserLogined = false;
    ui->cloudGroupBox->setEnabled(false);
    ui->userLabel->adjustSize();
    // 使 localGroupBox 可用而 cloudGroupBox不可用
//    on_localGroupBox_clicked(true);
//    on_cloudGroupBox_clicked(true);
//    on_localGroupBox_clicked(true);
    taskManager.init();
    for (const auto& task : taskManager.getTaskList()) {
        QTreeWidgetItem* taskItem = new QTreeWidgetItem;
        taskItem->setText(0, QFileInfo(task.backupFilename).fileName() + ".bak");
        taskItem->setText(1, task.nextTime.toString());
        taskItem->setText(2, task.frequency == 1 ? "每天" : "每周");
        taskItem->setText(3, task.password.trimmed() != "" ? "是" : "否");
        taskItem->setText(4, task.cloud ? "是" : "否");
        taskItem->setText(5, task.backupFilename + ".bak");
        ui->taskList->addTopLevelItem(taskItem);
    }
    if (!ui->taskList->currentItem() && ui->taskList->topLevelItemCount()) {
        ui->taskList->setCurrentItem(ui->taskList->topLevelItem(0));
    }
    connect(&timer, &QTimer::timeout, this, [ = ]() {
        if (taskManager.getTaskList().count()) {
            for (auto& task : taskManager.getTaskList()) {
                // if (task.nextTime.toString() == QDateTime::currentDateTime().toString()) {
                if (task.nextTime <= QDateTime::currentDateTime()) {
                    // 执行
                    /*QProcess tar;*/
                    QStringList files;
                    for (auto i : task.files) {
                        files.append(i);
                    }
                    /*auto rootDirectory = QFileInfo(task.files[0]).path();
                    auto currentDirectory = QDir::current();
                    QDir::setCurrent(rootDirectory);
                    tar.start(currentDirectory.path() + "/tar.exe", QStringList() << "-cvf" << QFileInfo(task.backupFilename).fileName() + ".tar" << files);
                    tar.waitForStarted(-1);
                    tar.waitForFinished(-1);*/
                    int errorCode = Pack::pack(files, QFileInfo(task.backupFilename).fileName() + ".tar");
                    if (errorCode) {
                        QStringList message = {"正常执行", "目标文件打开失败", "打开源文件失败"};
                        QMessageBox::information(this, "提示", message[errorCode],
                                                 QMessageBox::Yes, QMessageBox::Yes);
                        return;
                    }
                    Compressor compressor;
                    errorCode = compressor.compress(QFileInfo(task.backupFilename).fileName().toStdString() + ".tar",
                                                    QFileInfo(task.backupFilename).path().toStdString() + "/",
                                                    task.password.toStdString());
                    if (errorCode) {
                        QStringList message = {"正常执行", "源文件扩展名不是bak", "打开源文件失败", "打开目标文件失败"};
                        QMessageBox::information(this, "提示", message[errorCode],
                                                 QMessageBox::Yes, QMessageBox::Yes);
                        qDebug() << errorCode;
                        return;
                    }
                    QFile tarFile(QFileInfo(task.backupFilename).fileName() + ".tar");
                    tarFile.remove();
                    //QDir::setCurrent(currentDirectory.path());
                    // 云上传
                    if (task.cloud) {
                        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
                        QFile uploadFile(task.backupFilename + ".bak");
                        QNetworkRequest request(QUrl(api + "file/" + QFileInfo(task.backupFilename).fileName() + ".bak/"+this->currentUser));
                        request.setRawHeader("Content-Type", "application/bak");
                        uploadFile.open(QFile::ReadOnly);
                        QNetworkReply* reply = manager->put(request, uploadFile.readAll().toBase64());
                        uploadFile.close();
                        connect(manager, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply * _reply) {
                            if (_reply->readAll().toStdString() == "success") {
                                QMessageBox::information(this, "提示", "上传成功。",
                                                         QMessageBox::Yes, QMessageBox::Yes);
                            }
                        });
                        connect(reply, &QNetworkReply::uploadProgress, this, [](qint64 bytesReceived, qint64 bytesTotal) {
                            qDebug() << bytesReceived << "/" << bytesTotal;
                        });
                        connect(reply,
                                QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                                this,
                        [ = ](QNetworkReply::NetworkError code) {
                            if (code) {
                                QMessageBox::information(this, "提示", "上传失败。",
                                                         QMessageBox::Yes, QMessageBox::Yes);
                            }
                        });
                    } else {
                        QMessageBox::information(this, "提示", "备份成功。",
                                                 QMessageBox::Yes, QMessageBox::Yes);
                    }
                    // 更新下一次备份时间
                    int index = taskManager.getTaskList().indexOf(task);
                    while (task.nextTime <= QDateTime::currentDateTime()) {
                        taskManager.updateTime(index, task.nextTime.addDays(task.frequency == 1 ? 1 : 7));
                    }
                    ui->taskList->topLevelItem(index)->setText(1, task.nextTime.toString());
                    taskManager.writeJson();
                }
            }
        }
    });
    timer.start(1000);
}

Widget::~Widget() {
    on_clearFileButton_clicked();
    delete ui;
}


/*
void Widget::on_browseButton_clicked() {
    QString directory = QFileDialog::getExistingDirectory(
                            this,
                            tr("备份到"),
                            "/home",
                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory != "") {
        ui->backupFileDirectoryLineEdit->setText(directory);
    }
}
*/

void Widget::on_addFileButton_clicked() {
    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            "选择一个或多个文件",
                            "/home",
                            "所有文件 (*.*)");
    for (const auto& file : files) {
        // 去重
        bool duplication = false;
        for (int i = 0; i < ui->backupFileList->topLevelItemCount(); ++i) {
            if (ui->backupFileList->topLevelItem(i)->text(1) == file) {
                duplication = true;
                break;
            }
        }
        if (duplication) continue;

        QTreeWidgetItem* fileItem = new QTreeWidgetItem;
        fileItem->setText(0, QFileInfo(file).fileName());
        fileItem->setText(1, file);
        ui->backupFileList->addTopLevelItem(fileItem);
    }
    if (!ui->backupFileList->currentItem() && ui->backupFileList->topLevelItemCount()) {
        ui->backupFileList->setCurrentItem(ui->backupFileList->topLevelItem(0));
    }
}

void Widget::on_deleteFileButton_clicked() {
    if (ui->backupFileList->currentItem()) {
        delete ui->backupFileList->currentItem();
    }
}

void Widget::on_clearFileButton_clicked() {
    while (ui->backupFileList->currentItem()) {
        delete ui->backupFileList->currentItem();
    }
}

void Widget::on_addDirectoryButton_clicked() {
    QString directory = QFileDialog::getExistingDirectory(
                            this,
                            tr("选择一个文件夹"),
                            "/home",
                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory != "") {
        // 去重
        for (int i = 0; i < ui->backupFileList->topLevelItemCount(); ++i) {
            if (ui->backupFileList->topLevelItem(i)->text(1) == directory) return;
        }
        QTreeWidgetItem* fileItem = new QTreeWidgetItem;
        fileItem->setText(0, QFileInfo(directory).fileName());
        fileItem->setText(1, directory);
        ui->backupFileList->addTopLevelItem(fileItem);
    }
    if (!ui->backupFileList->currentItem() && ui->backupFileList->topLevelItemCount()) {
        ui->backupFileList->setCurrentItem(ui->backupFileList->topLevelItem(0));
    }
}

void Widget::setBackupConfig(
    QLineEdit* backupFilename,
    QLineEdit* backupFileDirectory,
    QRadioButton* noneRegular,
    QRadioButton* everyday,
    QRadioButton* everyweek,
    QCheckBox* passwordCheckBox,
    QLineEdit* password,
    QCheckBox* cloudBackup)
{
    this->backupFilename = backupFilename;
    this->backupFileDirectory = backupFileDirectory;
    this->noneRegular = noneRegular;
    this->everyday = everyday;
    this->everyweek = everyweek;
    this->passwordCheckBox = passwordCheckBox;
    this->password = password;
    this->cloudBackup = cloudBackup;
}

void Widget::on_startBackupButton_clicked() {
    BackupConfigDialog* backupconfigdialog = new BackupConfigDialog();
    backupconfigdialog->exec();
    // connect(backupconfigdialog, &BackupConfigDialog::backupConfig, this, &Widget::setBackupConfig);
    this->setBackupConfig(
            backupconfigdialog->backupFilename,
            backupconfigdialog->backupFileDirectory,
            backupconfigdialog->noneRegular,
            backupconfigdialog->everyday,
            backupconfigdialog->everyweek,
            backupconfigdialog->passwordCheckBox,
            backupconfigdialog->password,
            backupconfigdialog->cloudBackup);

    if (!ui->backupFileList->topLevelItemCount()) {
        QMessageBox::information(this, "提示", "请添加需要备份的文件。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if (this->backupFilename->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请输入备份后的文件名。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if (this->backupFileDirectory->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请输入备份保存到的目录。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    std::regex filenameExpress("[\\/:*?\"<>|]");
    if (std::regex_search(this->backupFilename->text().toStdString(), filenameExpress)) {
        QMessageBox::information(this, "提示", "请输入合法的备份文件名。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if (QFileInfo(this->backupFileDirectory->text() + "\\" + this->backupFilename->text()).exists()) {
        if (QMessageBox::Yes != QMessageBox::question(this, "警告", "文件已存在，确认覆盖？", QMessageBox::Yes | QMessageBox::No)) {
            return;
        }
    }
    if (this->passwordCheckBox->isChecked() && this->password->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请输入密码。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }


    // 都与第一个文件的目录对比
    auto rootDirectory = QFileInfo(ui->backupFileList->topLevelItem(0)->text(1)).path();
    for (int i = 1; i < ui->backupFileList->topLevelItemCount(); ++i) {
        if (QFileInfo(ui->backupFileList->topLevelItem(i)->text(1)).path() != rootDirectory) {
            QMessageBox::information(this, "提示", "选择的文件或文件夹应位于同一目录下。",
                                     QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
    }

    if (!this->noneRegular->isChecked()) {
        QTreeWidgetItem* taskItem = new QTreeWidgetItem;
        taskItem->setText(0, this->backupFilename->text() + ".bak");
        auto nextTime = QDateTime::currentDateTime().addDays(this->everyday->isChecked() ? 1 : 7);
        taskItem->setText(1, nextTime.toString());
        taskItem->setText(2, this->everyday->isChecked() ? "每天" : "每周");
        taskItem->setText(3, this->passwordCheckBox->isChecked() ? "是" : "否");
        taskItem->setText(4, this->cloudBackup->isChecked() ? "是" : "否");
        taskItem->setText(5, this->backupFileDirectory->text() + "/" +  this->backupFilename->text() + ".bak");
        ui->taskList->addTopLevelItem(taskItem);
        QList<QString> files;
        for (int i = 0; i < ui->backupFileList->topLevelItemCount(); ++i) {
            files.append(ui->backupFileList->topLevelItem(i)->text(1));
        }
        taskManager.addTask(Task(files,
                                 this->backupFileDirectory->text() + "/" + this->backupFilename->text(),
                                 this->everyday->isChecked() ? 1 : 2,
                                 this->password->text(),
                                 this->cloudBackup->isChecked(),
                                 nextTime));
        if (!ui->taskList->currentItem() && ui->taskList->topLevelItemCount()) {
            ui->taskList->setCurrentItem(ui->taskList->topLevelItem(0));
        }
    }
    // 调用打包压缩加密
    /*QProcess tar;*/
    QStringList files;
    for (int i = 0; i < ui->backupFileList->topLevelItemCount(); ++i) {
        files.append(ui->backupFileList->topLevelItem(i)->text(1));
    }
    /*auto currentDirectory = QDir::current();
    QDir::setCurrent(rootDirectory);
    tar.start(currentDirectory.path() + "/tar.exe", QStringList() << "-cvf" << this->backupFilename->text() + ".tar" << files);
    tar.waitForStarted(-1);
    tar.waitForFinished(-1);*/
    int errorCode = Pack::pack(files, this->backupFilename->text() + ".tar");
    if (errorCode) {
        QStringList message = {"正常执行", "目标文件打开失败", "打开源文件失败"};
        QMessageBox::information(this, "提示", message[errorCode],
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    Compressor compressor;
    errorCode = compressor.compress(this->backupFilename->text().toStdString() + ".tar",
                                    this->backupFileDirectory->text().toStdString() + "/",
                                    this->passwordCheckBox->isChecked() ? this->password->text().toStdString() : "");
    if (errorCode) {
        QStringList message = {"正常执行", "源文件扩展名不是bak", "打开源文件失败", "打开目标文件失败"};
        QMessageBox::information(this, "提示", message[errorCode],
                                 QMessageBox::Yes, QMessageBox::Yes);
        qDebug() << errorCode;
        return;
    }
    QFile tarFile(this->backupFilename->text() + ".tar");
    //添加代码：在备份后创建包含文件目录的.json文件，以供备份检验时使用
    tarFile.remove();
    QFile temjson(this->backupFileDirectory->text() + "/" + this->backupFilename->text() + ".json");
    if(!temjson.open(QIODevice::ReadWrite)) {
            qDebug() << "json file open error";
        } else {
            qDebug() <<"json file open!";
        }
    temjson.resize(0);
    QJsonArray jsonArray;
    for(const auto& file0 : files){
        jsonArray.append(file0);
    }
    QJsonDocument jsonDoc;
    jsonDoc.setArray(jsonArray);
    temjson.write(jsonDoc.toJson());
    temjson.close();
    qDebug() << "Write to json file";
    //以上*********************************************************(已验证）

    //QDir::setCurrent(currentDirectory.path());
    // 云上传
    if (this->cloudBackup->isChecked()) {
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QFile uploadFile(this->backupFileDirectory->text() + "/" + this->backupFilename->text() + ".bak");
        QNetworkRequest request(QUrl(api + "file/" + this->backupFilename->text() + ".bak/"+this->currentUser));
        request.setRawHeader("Content-Type", "application/bak");
        uploadFile.open(QFile::ReadOnly);
        QNetworkReply* reply = manager->put(request, uploadFile.readAll().toBase64());
        uploadFile.close();
        connect(manager, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply * _reply) {
            if (_reply->readAll().toStdString() == "success") {
                QMessageBox::information(this, "提示", "上传成功。",
                                         QMessageBox::Yes, QMessageBox::Yes);
            }
        });
        connect(reply, &QNetworkReply::uploadProgress, this, [](qint64 bytesReceived, qint64 bytesTotal) {
            qDebug() << bytesReceived << "/" << bytesTotal;
        });
        connect(reply,
                QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                this,
        [ = ](QNetworkReply::NetworkError code) {
            if (code) {
                QMessageBox::information(this, "提示", "上传失败。",
                                         QMessageBox::Yes, QMessageBox::Yes);
            }
        });
    } else {
        QMessageBox::information(this, "提示", "备份成功。",
                                 QMessageBox::Yes, QMessageBox::Yes);
    }
}

//void Widget::on_localGroupBox_clicked(bool checked) {
//    ui->cloudGroupBox->setChecked(false);
//    ui->localGroupBox->setChecked(true);
//}
/*
void Widget::on_cloudGroupBox_clicked(bool checked) {
    ui->cloudFileRestoreLineEdit->setEnabled(false);
    ui->cloudFileList->clear();
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(api + "filelist/"+this->currentUser));
    QNetworkReply* reply = manager->get(request);
    connect(manager, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply * _reply) {
        auto filelist = QJsonDocument::fromJson(_reply->readAll()).array();
        for (auto file : filelist) {
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, file.toString());
            ui->cloudFileList->addTopLevelItem(item);
        }
    });
    connect(reply,
            QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this,
    [ = ](QNetworkReply::NetworkError code) {
        if (code) {
            QMessageBox::information(this, "提示", "拉取云端列表失败。",
                                     QMessageBox::Yes, QMessageBox::Yes);
        }
    });
}
*/
void Widget::on_browseLocalFile_clicked() {
    auto file = QFileDialog::getOpenFileName(
                    this,
                    "选择一个备份文件",
                    "",
                    "所有文件 (*.*)");
    if (file != "") {
        ui->localFileRestoreLineEdit->setText(file);
    }
}

void Widget::on_cloudFileList_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    if (current)
        ui->cloudFileRestoreLineEdit->setText(current->text(0));
}

void Widget::on_browseRestoreDirectoryButton_clicked() {
    QString directory = QFileDialog::getExistingDirectory(
                            this,
                            tr("恢复到"),
                            "/home",
                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory != "") {
        ui->backupFileRestoreDirectoryLineEdit->setText(directory);
    }
}

void Widget::on_passwordCheckBox_2_stateChanged(int arg1) {
    ui->passwordLineEdit_2->setEnabled(ui->passwordCheckBox_2->checkState());
}

void Widget::on_deleteTaskButton_clicked() {
    if (ui->taskList->currentItem()) {
        taskManager.removeTask(ui->taskList->indexOfTopLevelItem(ui->taskList->currentItem()));
        delete ui->taskList->currentItem();
        if (!ui->taskList->currentItem() && ui->taskList->topLevelItemCount()) {
            ui->taskList->setCurrentItem(ui->taskList->topLevelItem(0));
        }
    }
}

void Widget::on_clearTaskButton_clicked() {
    ui->taskList->clear();
    taskManager.clear();
}

void Widget::on_taskList_customContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* currentItem = ui->taskList->itemAt(pos);
    if (currentItem) {
        int index = ui->taskList->indexOfTopLevelItem(currentItem);
        popMenu = new QMenu(this);
        openFolder = popMenu->addAction("打开备份文件所在目录");
        check = popMenu->addAction("与原文件校验");
        connect(openFolder, &QAction::triggered, this, [ = ]() {
            QDesktopServices::openUrl(QUrl("file:///" + QFileInfo(currentItem->text(5)).path(), QUrl::TolerantMode));
        });
        connect(check, &QAction::triggered, this, [ = ]() {
            Decompressor decompressor;
            QDir dir;
            dir.mkdir("./TEMP");
            QFileInfo TEMP("./TEMP");
            int errorCode = decompressor.decompress(currentItem->text(5).toStdString(),
                                                    TEMP.absoluteFilePath().toStdString() + "/",
                                                    taskManager.getTaskList()[index].password.toStdString());
            if (errorCode) {
                QStringList message = {"正常执行", "源文件扩展名不是bak", "打开源文件失败", "打开目标文件失败", "文件过短，频率表不完整", "文件结尾不完整", "密码错误", "解码错误"};
                QMessageBox::information(this, "提示", message[errorCode],
                                         QMessageBox::Yes, QMessageBox::Yes);
                dir.rmdir("./TEMP");
                return;
            }
            /*QProcess tar;
            auto currentDirectory = QDir::current();
            QDir::setCurrent("./TEMP");*/
            QString tempFilename = "./TEMP/" + QFileInfo(currentItem->text(5)).fileName();
            tempFilename = tempFilename.left(tempFilename.length() - 3) + "tar";
            qDebug() << tempFilename;
            errorCode = Unpack::unpack(QFileInfo(tempFilename).absoluteFilePath(), "./TEMP");
            if (errorCode) {
                QStringList message = {"正常执行",  "打开源文件失败", "目标文件打开失败", "创建目录失败"};
                QMessageBox::information(this, "提示", message[errorCode],
                                         QMessageBox::Yes, QMessageBox::Yes);
                return;
            }
            /*tar.start(currentDirectory.path() + "/tar.exe", QStringList() << "-xvf" << tempFilename);
            tar.waitForStarted(-1);
            tar.waitForFinished(-1);*/
            QFile tarFile(tempFilename);
            tarFile.remove();
            /*QDir::setCurrent(currentDirectory.path());*/
            qDebug() << "!!!";
            auto difference = Check::check(taskManager.getTaskList()[index].files, "./TEMP");
            if (difference.empty()) {
                QMessageBox::information(this, "提示", "备份无差异",
                                         QMessageBox::Yes, QMessageBox::Yes);
            } else {
                QMessageBox::information(this, "提示", "备份有差异",
                                         QMessageBox::Yes, QMessageBox::Yes);
                QFile diff("diff.txt");
                diff.open(QFile::WriteOnly);
                QStringList types = {"删除", "修改", "增加"};
                for (auto& d : difference) {
                    auto filename = d.first;
                    auto type = d.second;
                    diff.write(types[type].toStdString().c_str());
                    diff.write(" ");
                    diff.write(filename.toStdString().c_str());
                    diff.write("\n");
                }
                diff.close();
                QProcess notepad;
                notepad.startDetached("notepad diff.txt");
            }
            dir.rmdir("./TEMP");
        });
        popMenu->exec(QCursor::pos());
    }
}

void Widget::on_startRestoreButton_clicked() {
    if (ui->localGroupBox->isChecked() && ui->localFileRestoreLineEdit->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请选择要恢复的备份文件。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if (ui->backupFileRestoreDirectoryLineEdit->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请选择要恢复到的目录。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if (ui->passwordCheckBox_2->isChecked() && ui->passwordLineEdit_2->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请输入密码。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    // 去除checkbox，本地恢复过程
    Decompressor decompressor;
    int errorCode = decompressor.decompress(ui->localFileRestoreLineEdit->text().toStdString(),
                                            ui->backupFileRestoreDirectoryLineEdit->text().toStdString() + "/",
                                            ui->passwordCheckBox_2->isChecked() ? ui->passwordLineEdit_2->text().toStdString() : "");
    if (errorCode) {
        QStringList message = {"正常执行", "源文件扩展名不是bak", "打开源文件失败", "打开目标文件失败", "文件过短，频率表不完整", "文件结尾不完整", "密码错误", "解码错误"};
        QMessageBox::information(this, "提示", message[errorCode],
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    /*QProcess tar;
        auto currentDirectory = QDir::current();
        QDir::setCurrent(ui->backupFileRestoreDirectoryLineEdit->text());*/
    QString tempFilename = ui->backupFileRestoreDirectoryLineEdit->text() + "/" + QFileInfo(ui->localFileRestoreLineEdit->text()).fileName();
    tempFilename = tempFilename.left(tempFilename.length() - 3) + "tar";
    /*tar.start(currentDirectory.path() + "/tar.exe", QStringList() << "-xvf" << tempFilename);
        tar.waitForStarted(-1);
        tar.waitForFinished(-1);*/
    errorCode = Unpack::unpack(tempFilename, ui->backupFileRestoreDirectoryLineEdit->text());
    if (errorCode) {
        QStringList message = {"正常执行",  "打开源文件失败", "目标文件打开失败", "创建目录失败"};
        QMessageBox::information(this, "提示", message[errorCode],
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    QFile tarFile(tempFilename);
    tarFile.remove();
    //备份文件恢复后，对原文件与恢复文件进行比对
    QFile file1(ui->localFileRestoreLineEdit->text().replace(".bak" , "") + ".json");
    if (!file1.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "can't open error!";
        QMessageBox::information(this, "提示", "该备份文件的.json文件不存在",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    QJsonParseError jsonParserError;
    QJsonDocument   jsonDocument =	QJsonDocument::fromJson(file1.readAll(), &jsonParserError);
    QList<QString> listfromjson;
    if (!jsonDocument.isNull() &&jsonParserError.error == QJsonParseError::NoError)
    {
        qDebug() << "文件解析成功\n";
        if (jsonDocument.isArray())
        {
            QJsonArray array = jsonDocument.array();  // 转数组
            int nSize = array.size();
            for (int i = 0; i < nSize; ++i) {  // 遍历数组
                QJsonValue value = array.at(i);
                if (value.type() == QJsonValue::String) {
                    listfromjson<<value.toString();
                }
            }
        }
    }
    file1.close();
    auto difference = Check::check(listfromjson, ui->backupFileRestoreDirectoryLineEdit->text());
    if (difference.empty()) {
        QMessageBox::information(this, "提示", "备份无差异",
                                 QMessageBox::Yes, QMessageBox::Yes);
    } else {
        QMessageBox::information(this, "提示", "备份有差异",
                                 QMessageBox::Yes, QMessageBox::Yes);
        QFile diff("diff.txt");
        diff.open(QFile::WriteOnly);
        QStringList types = {"删除", "修改", "增加"};
        for (auto& d : difference) {
            auto filename = d.first;
            auto type = d.second;
            diff.write(types[type].toStdString().c_str());
            diff.write(" ");
            diff.write(filename.toStdString().c_str());
            diff.write("\n");
        }
        diff.close();
        QProcess notepad;
        notepad.startDetached("notepad diff.txt");
    }
    //以上***********************************************************(在正常情况下测试完成）

    /*QDir::setCurrent(currentDirectory.path());*/
    QMessageBox::information(this, "提示", "恢复完成。",
                             QMessageBox::Yes, QMessageBox::Yes);
    QDesktopServices::openUrl(QUrl("file:///" + ui->backupFileRestoreDirectoryLineEdit->text(), QUrl::TolerantMode));
}

void Widget::on_cloudFileList_customContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* currentItem = ui->cloudFileList->itemAt(pos);
    if (currentItem) {
        popMenu = new QMenu(this);
        removeCloudBackupFile = popMenu->addAction("删除该云备份");
        connect(removeCloudBackupFile, &QAction::triggered, this, [ = ]() {
            if (QMessageBox::Yes != QMessageBox::question(this, "警告", "确认删除？", QMessageBox::Yes | QMessageBox::No)) {
                return;
            }
            QNetworkAccessManager* manager = new QNetworkAccessManager(this);
            QNetworkRequest request(QUrl(api + "file/" + currentItem->text(0)+"/"+this->currentUser));
            QNetworkReply* reply = manager->deleteResource(request);
            connect(manager, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply * _reply) {
                if (_reply->readAll().toStdString() == "success") {
                    QMessageBox::information(this, "提示", "删除成功。",
                                             QMessageBox::Yes, QMessageBox::Yes);
                    if (ui->cloudFileList->topLevelItemCount()) {
                        ui->cloudFileList->setCurrentItem(ui->cloudFileList->topLevelItem(0));
                    }
                }
            });
            connect(reply,
                    QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                    this,
            [ = ](QNetworkReply::NetworkError code) {
                if (code) {
                    QMessageBox::information(this, "提示", "删除失败。",
                                             QMessageBox::Yes, QMessageBox::Yes);
                }
            });
        });
        popMenu->exec(QCursor::pos());
    }
}

void Widget::on_mainWindowLoginButton_clicked()
{
    /*
    connect(usermanager, &UserManager::sendUsername, this, [&] (QString username) {
        this->currentUser=username;
        if(this->currentUser!="") {
            ui->mainWindowLoginButton->hide();
            ui->mainWindowQuitButton->show();
            ui->userLabel->setText(this->currentUser);
        }
    });
    */
    UserManager* usermanager = new UserManager(this);
    usermanager->exec();
    if(usermanager->loginState) {
        ui->mainWindowLoginButton->hide();
        ui->mainWindowQuitButton->show();
        this->currentUser=usermanager->username;
        ui->cloudGroupBox->setEnabled(usermanager->loginState);
        ui->userLabel->setText("当前用户："+this->currentUser);
        ui->userLabel->adjustSize();
        ui->userLabel->show();
    }
}


void Widget::on_mainWindowQuitButton_clicked()
{
    this->currentUser="";
    ui->mainWindowLoginButton->show();
    ui->mainWindowQuitButton->hide();
    ui->cloudGroupBox->setEnabled(false);
    ui->userLabel->setText("未登录");
    ui->userLabel->adjustSize();
}


void Widget::on_startCloudRestoreButton_clicked()
{
//    if(this->isUserLogined == false) {
//        QMessageBox::information(this, "提示", "用户未登录，无法进行云端操作");
//        ui->cloudGroupBox->setEnabled(false);
//    }
    if (ui->cloudFileRestoreLineEdit->text().trimmed() == "") {
        QMessageBox::information(this, "提示", "请选择要恢复的备份文件。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(api + "file/" + ui->cloudFileRestoreLineEdit->text()+"/"+this->currentUser));
    QNetworkReply* reply = manager->get(request);
    connect(manager, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply * _reply) {
        auto file = QByteArray::fromBase64(_reply->readAll());
        qDebug() << file.size();
        QFile cloudFile("temp_" + ui->cloudFileRestoreLineEdit->text());
        cloudFile.open(QFile::WriteOnly);
        cloudFile.write(file);
        cloudFile.close();
        Decompressor decompressor;
        int errorCode = decompressor.decompress("temp_" + ui->cloudFileRestoreLineEdit->text().toStdString(),
                                                ui->cloudBackupFileRestoreDirectoryLineEdit->text().toStdString() + "/",
                                                ui->passwordCheckBox_3->isChecked() ? ui->passwordLineEdit_3->text().toStdString() : "");
        if (errorCode) {
            QStringList message = {"正常执行", "源文件扩展名不是bak", "打开源文件失败", "打开目标文件失败", "文件过短，频率表不完整", "文件结尾不完整", "密码错误", "解码错误"};
            QMessageBox::information(this, "提示", message[errorCode],
                                     QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
        cloudFile.remove();
        /*QProcess tar;
            auto currentDirectory = QDir::current();*/
        //QDir::setCurrent(ui->backupFileRestoreDirectoryLineEdit->text());
        QString tempFilename = ui->cloudBackupFileRestoreDirectoryLineEdit->text() + "/" + "temp_" + ui->cloudFileRestoreLineEdit->text().left(ui->cloudFileRestoreLineEdit->text().length() - 3) + "tar";
        /*tar.start(currentDirectory.path() + "/tar.exe", QStringList() << "-xvf" << tempFilename);
            tar.waitForStarted(-1);
            tar.waitForFinished(-1);*/
        errorCode = Unpack::unpack(tempFilename, ui->cloudBackupFileRestoreDirectoryLineEdit->text());
        if (errorCode) {
            QStringList message = {"正常执行",  "打开源文件失败", "目标文件打开失败", "创建目录失败"};
            QMessageBox::information(this, "提示", message[errorCode],
                                     QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
        QFile tarFile(tempFilename);
        tarFile.remove();
        /*QDir::setCurrent(currentDirectory.path());*/
        QMessageBox::information(this, "提示", "恢复完成。",
                                 QMessageBox::Yes, QMessageBox::Yes);
        QDesktopServices::openUrl(QUrl("file:///" + ui->cloudBackupFileRestoreDirectoryLineEdit->text(), QUrl::TolerantMode));
    });
    connect(reply,
            QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this,
            [ = ](QNetworkReply::NetworkError code) {
                if (code) {
                    QMessageBox::information(this, "提示", "云备份下载失败。",
                                             QMessageBox::Yes, QMessageBox::Yes);
                }
            });
}


void Widget::on_pushButton_clicked()
{
    ui->cloudFileRestoreLineEdit->setEnabled(false);
    ui->cloudFileList->clear();
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(api + "filelist/"+this->currentUser));
    QNetworkReply* reply = manager->get(request);
    connect(manager, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply * _reply) {
        auto filelist = QJsonDocument::fromJson(_reply->readAll()).array();
        for (auto file : filelist) {
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, file.toString());
            ui->cloudFileList->addTopLevelItem(item);
        }
    });
    connect(reply,
            QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this,
            [ = ](QNetworkReply::NetworkError code) {
                if (code) {
                    QMessageBox::information(this, "提示", "拉取云端列表失败。",
                                             QMessageBox::Yes, QMessageBox::Yes);
                }
            });
}


void Widget::on_cloudBrowseRestoreDirectoryButton_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("恢复到"),
        "/home",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory != "") {
        ui->cloudBackupFileRestoreDirectoryLineEdit->setText(directory);
    }
}


void Widget::on_passwordCheckBox_3_stateChanged(int arg1)
{
    ui->passwordLineEdit_3->setEnabled(ui->passwordCheckBox_3->checkState());
}

