#include "mainwindow.h"
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QThread>
#include <QSharedMemory>
#include <QTime>
#include <unistd.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;

    if (getuid() != 0) {
        //随机生成一个key
        QString key = QString("ch341pgr%1").arg(
                        (QTime::currentTime().msec() + QTime::currentTime().second() * 131));
        QSharedMemory *sharedMemory = new QSharedMemory;
        sharedMemory->setKey(key);
        if (!sharedMemory->create(key.length(), QSharedMemory::ReadWrite)) {
            //不会出现，除非bug
            delete sharedMemory;
            exit(0);
        }

        QString pwd;
        QInputDialog *input = new QInputDialog;
        input->resize(300, 300);
        input->setWindowTitle(QObject::tr("此程序需要授权"));
        input->setLabelText(QObject::tr("点击“取消”将以普通用户运行\n请输入密码:"));
        input->setTextEchoMode(QLineEdit::Password);
        input->setCancelButtonText(QObject::tr("取消"));
        input->setOkButtonText(QObject::tr("确定"));

        QProcess process(&w);
        QStringList args;
        QByteArray msg;
        msg.resize(key.size());
        int errCount = 0;
        while (errCount < 3) {
            int btn = input->exec();

            if (QInputDialog::Rejected == btn) {
                QMessageBox::warning(&w, QObject::tr("警告"), QObject::tr("程序未获得授权，可能无法正常工作"));
                break;
            }

            pwd = input->textValue();
            args.clear();

            if (!pwd.isEmpty()) {
                args << "-c";

                //传递共享内存的key值给新进程
                args << "echo '" + pwd + "'" + " | sudo -S " +
                        QApplication::applicationFilePath() +
                        " " + key +
                        " " + w.homeDir() +
                        " " + QString::number(getuid()) +
                        " " + QString::number(getgid());

                process.setProgram("/bin/bash");
                process.setArguments(args);
                process.startDetached();
                process.waitForFinished(1000);

                QThread::msleep(1000); //等待 1 秒，让新进程写入数据
                sharedMemory->lock();
                memcpy(msg.data(), sharedMemory->data(), key.size());
                sharedMemory->unlock();

                if (QString::fromLocal8Bit(msg) == key) {
                    //密码正确 成功启动
                    delete input;
                    sharedMemory->detach();
                    delete sharedMemory;
                    exit(0);
                }
            }

            if (++errCount >= 3) {
                QMessageBox::warning(&w, QObject::tr("密码错误"), QObject::tr("密码错误，请以 root 权限运行"));
                delete input;
                sharedMemory->detach();
                delete sharedMemory;
                exit(0);
            }

            input->setLabelText(QObject::tr("还剩 %1 次机会，请输入密码:").arg(3 - errCount));
        }

        delete input;
        sharedMemory->detach();
        delete sharedMemory;
        //run in normal user mode
    }else {
        if (argc > 1) { //是我们启动的进程
            QString key = QString::fromLocal8Bit(argv[1]);
            QSharedMemory *sharedMemory = new QSharedMemory;
            sharedMemory->setKey(key);

            if (sharedMemory->attach(QSharedMemory::ReadWrite)) {
                sharedMemory->lock();
                memcpy(sharedMemory->data(), key.toLocal8Bit(), key.size());
                sharedMemory->unlock();
                //解绑，让系统可以回收资源
                sharedMemory->detach();
            }

            delete sharedMemory;

            w.setHomeDir(argv[2]);
            w.setUid(QString(argv[3]).toUInt());
            w.setGid(QString(argv[4]).toUInt());
        }
    }

    w.show();
    w.onloaded();
    return a.exec();
}
