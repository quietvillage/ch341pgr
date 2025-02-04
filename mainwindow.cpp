#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chipdata.h"
#include "version.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_croppingDialog(nullptr)
    , m_uid(0)
{
    ui->setupUi(this);
    this->setWindowTitle(QString("ch341pgr - v%1").arg(CH341PGR_VERSION_STRING));
    this->setWindowIcon(QIcon(":logo.png"));
    QFont font = this->font();
    font.setPointSizeF(10.5);
    this->setFont(font);

    this->on_btn_flush_clicked();
    this->onMemTypeChanged(0);
    m_homeDir = QDir::homePath();
    m_translator.load(QCoreApplication::applicationDirPath() + "/translations/ch341pgr_en.qm");

    ui->centralwidget->setLayout(ui->hLayout);
    ui->progressBar->setParent(ui->hexView);
    ui->progressBar->setVisible(false);

    connect(ui->btn_open, SIGNAL(clicked()), this, SLOT(on_action_open_triggered()));
    connect(ui->btn_save, SIGNAL(clicked()), this, SLOT(on_action_save_triggered()));
    connect(ui->btn_cropping, SIGNAL(clicked()), this, SLOT(on_action_cropping_triggered()));
    connect(ui->combo_memType, SIGNAL(currentIndexChanged(int)), this, SLOT(onMemTypeChanged(int)));
    connect(ui->combo_ports, SIGNAL(currentIndexChanged(int)), this, SLOT(onPortChanged(int)));
    connect(ui->combo_model, SIGNAL(currentIndexChanged(int)), this, SLOT(onModelChanged(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_croppingDialog;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    ui->progressBar->move((ui->hexView->width() - ui->progressBar->width()) >> 1,
                          (ui->hexView->height() - ui->progressBar->height()) >> 1);

    if (e != nullptr) {
        e->ignore();
    }
}

void MainWindow::onloaded()
{
    this->resizeEvent(nullptr);
}

void MainWindow::on_action_open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"), "" == m_file? m_homeDir : m_file, tr("bin文件(*.bin *.rom);;全部文件(*)"));
    if ("" == fileName) {return;}

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开文件“%1”").arg(fileName), tr("确定"));
        return;
    }

    ui->hexView->setData(file.readAll());
    file.close();
    m_file = fileName;
    ui->statusbar->showMessage(tr("已选择文件“%1”").arg(m_file), 3000);
}


void MainWindow::on_action_save_triggered()
{
    if (!ui->hexView->data().length()) {
        QMessageBox::information(this, tr("提示"), tr("缓冲区没有内容"), tr("确定"));
        return;
    }

    QFileInfo info(m_file);
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存文件"), (info.path() == "" ? m_homeDir : info.path()) + tr("/新建文件"), tr("bin文件(*.bin *.rom);;全部文件(*)"));
    if ("" == fileName) {return;}

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开文件“%1”").arg(fileName), tr("确定"));
        return;
    }

    ui->hexView->setBusy(true);
    file.write(ui->hexView->data());
    ui->hexView->setBusy(false);
    file.close();

    if (getuid() != m_uid) { //非root用户
        chown(fileName.toLocal8Bit(), m_uid, m_gid);
    }

    QMessageBox::information(this, tr("提示"), tr("保存完成"), tr("确定"));
}


void MainWindow::on_action_quit_triggered()
{
    this->close();
}


void MainWindow::on_btn_flush_clicked()
{
    QStringList portList;
    int ret, i, count;
    ui->combo_ports->clear();
    m_portList.clear();
    ret = libusb_init(nullptr);
    if (ret < 0) {
        return;
    }

    struct libusb_device **devices;
    struct libusb_device_descriptor devDescriptor;
    count = libusb_get_device_list(nullptr, &devices);

    for (i = 0; i < count; ++i) {
        ret = libusb_get_device_descriptor(devices[i], &devDescriptor);
        if (ret < 0) {
            goto _exit;
        }

        if (CH341_VENDOR_ID == devDescriptor.idVendor &&
                CH341_PRODUCT_ID == devDescriptor.idProduct)
        {
            ret = libusb_get_port_number(devices[i]);
            portList << QString("USB %1").arg(ret);
            m_portList << QPair(libusb_get_bus_number(devices[i]), ret);
        }
    }

    if (portList.size()) {
        ui->combo_ports->addItems(portList);
        m_port.setPort(m_portList.at(ui->combo_ports->currentIndex()));
    }

_exit:
    libusb_free_device_list(devices, 1);
    libusb_exit(nullptr);
}


void MainWindow::on_btn_detect_clicked()
{
    if (ui->combo_memType->currentIndex()) {
        QMessageBox::information(this, tr("提示"), tr("自动检测功能仅支持 25 系列存储器"), tr("确定"));
        return;
    }

    if (!m_port.openDevice()) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    QByteArray info = m_port.spiJedecId();
    if (info.length() >= 3) {
        switch ((uchar)info.at(0)) {
        case 0xEF: ui->combo_vendor->setCurrentText(tr("Winbond")); break;
        case 0xC2: ui->combo_vendor->setCurrentText(tr("MXIC")); break;
        default: ui->combo_vendor->setCurrentText(tr("未知")); break;
        }

        ui->label_code->setText("0x" + QString("%1").arg((uchar)info.at(0), 2, 16, QLatin1Char('0')).toUpper()
                                + "  0x" + QString("%1").arg((uchar)info.at(1), 2, 16, QLatin1Char('0')).toUpper()
                                + "  0x" + QString("%1").arg((uchar)info.at(2), 2, 16, QLatin1Char('0')).toUpper()
                                );

        if ((uchar)info.at(2) >= 0x11 && (uchar)info.at(2) <= 0x1B) {
            if ((uchar)info.at(2) < 0x14) {
                ui->label_size->setText(tr("容量： %1KBytes").arg(1 << ((uchar)info.at(2) - 0x11 + 7)));
            }else {
                ui->label_size->setText(tr("容量： %1MBytes").arg(1 << ((uchar)info.at(2) - 0x14)));
            }

            ui->combo_model->setCurrentIndex((uchar)info.at(2) - 0x10);
        }else {
            ui->label_size->setText(tr("容量：(未知)"));
            ui->combo_model->setCurrentIndex(0);
        }
    }else {
        QMessageBox::critical(this, tr("错误"), tr("无法检测芯片"), tr("确定"));
        ui->label_code->setText("");
        ui->label_size->setText(tr("容量："));
    }

    m_port.closeDevice();
}

void MainWindow::on_btn_erase_clicked()
{
    if (ui->combo_memType->currentIndex()) {
        QMessageBox::information(this, tr("提示"), tr("EEPROM可直接写任何数据，不需要擦除"), tr("确定"));
        return;
    }

    this->initModel();

    if (!m_port.openDevice()) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    ui->statusbar->showMessage(tr("请稍后..."));
    ui->statusbar->repaint();

    if(m_port.spiEraseChip()) {
        QMessageBox::information(this, tr("提示"), tr("擦除完成"), tr("确定"));
    }else {
        QMessageBox::critical(this, tr("错误"), tr("擦除失败"), tr("确定"));
    }

    m_port.closeDevice();
    ui->statusbar->showMessage("");
}

//返回 0 - 未指定型号
int MainWindow::chipSize()
{
    int i = ui->combo_model->currentIndex();
    if (!i) {
        QMessageBox::information(this, tr("提示"), tr("请指定芯片型号"), tr("确定"));
        return 0;
    }

    if (ui->combo_memType->currentIndex() == 0) {
        return 1024 * 128 * (1 << (i - 1));
    }else {
        return 128 * (1 << (i - 1));
    }
}

void MainWindow::initModel()
{
    if (ui->combo_memType->currentIndex() == 0) {
        m_port.setChipModel(Ch341Interface::SpiFlash, ui->combo_model->currentIndex());
    }else {
        m_port.setChipModel(Ch341Interface::I2cEEPROM, ui->combo_model->currentIndex());
    }
}

void MainWindow::blockActions(bool block)
{
    block = !block;
    ui->btn_read->setEnabled(block);
    ui->btn_write->setEnabled(block);
    ui->btn_erase->setEnabled(block);
    ui->btn_checkEmpty->setEnabled(block);
    ui->btn_check->setEnabled(block);
    ui->btn_detect->setEnabled(block);
    ui->btn_open->setEnabled(block);
    ui->action_open->setEnabled(block);
    ui->btn_cropping->setEnabled(block);
    ui->action_cropping->setEnabled(block);
}

void MainWindow::on_btn_checkEmpty_clicked()
{
    int size, j, step = CH341_MAX_BUF_LEN; //一次读取 4KB
    if (!(size = this->chipSize())) {return;}

    this->initModel();
    int i = m_port.openDevice();
    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    QByteArray stepData;
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    this->blockActions(true);

    int errCount = 0;
    i = 0;
    while (i < size) {
        if (size - i < step) {
            step = size - i;
        }
        stepData = m_port.read(step, i);
        if (stepData.length() != step) {
            ++errCount;
            if (errCount < 5) {continue;} //连续 5 次出错
            QMessageBox::critical(this, tr("错误"), m_port.errorMessage(Ch341Interface::ReadError), tr("确定"));
            goto final;
        }

        j = step;
        while (j--) {
            if ((uchar)stepData.at(j) != 0xFF) {
                QMessageBox::critical(this, tr("提示"), tr("存储器内容不为空"), tr("确定"));
                goto final;
            }
        }

        errCount = 0;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

    QMessageBox::information(this, tr("提示"), tr("存储器内容是空的"), tr("确定"));

final:
    m_port.closeDevice();
    ui->progressBar->setVisible(false);
    this->blockActions(false);
}


void MainWindow::on_btn_read_clicked()
{
    QByteArray buffer;
    int size, step = CH341_MAX_BUF_LEN; //一次读取 4KB
    if (!(size = this->chipSize())) {return;}
    this->initModel();

    int i = m_port.openDevice();

    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    QByteArray stepData;

    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    this->blockActions(true);

    int errCount = 0;
    i = 0;
    while (i < size) {
        if (size - i < step) {
            step = size - i;
        }

        stepData = m_port.read(step, i);
        if (stepData.length() != step) {
            ++errCount;
            if (errCount < 5) {continue;}
            QMessageBox::critical(this, tr("错误"), m_port.errorMessage(Ch341Interface::ReadError), tr("确定"));
            goto final;
        }

        errCount = 0;
        buffer += stepData;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

    ui->hexView->setData(buffer);

final:
    m_port.closeDevice();
    ui->progressBar->setVisible(false);
    this->blockActions(false);
}


void MainWindow::on_btn_write_clicked()
{
    if (!ui->hexView->data().length()) {
        QMessageBox::information(this, tr("提示"), tr("缓冲区没有内容"), tr("确定"));
        return;
    }

    int size, step = CH341_MAX_BUF_LEN; // CH341_MAX_BUF_LEN = sector size = 4KB
    if (!(size = this->chipSize())) {return;}
    this->initModel();

    int i = m_port.openDevice(), sub;
    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    if (ui->hexView->data().length() > size) {
        i = QMessageBox::question(this, tr("确认"), tr("缓冲区长度已超过存储器容量，超过部分将被直接丢弃，是否继续"), tr("否"), tr("是"));
        if (0 == i) {return;}
    }else {
        size = ui->hexView->data().length();
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->hexView->setBusy(true);
    this->blockActions(true);

    int errCount = 0;
    i = 0;
    while (i < size) {
        if (step > size - i) {
            step = size - i;
        }

        sub = m_port.write(ui->hexView->data().mid(i, step), i);
        if (sub != step) {
            ++errCount;
            if (errCount < 5) {
                m_port.spiEraseSector(i); // step = CH341_MAX_BUF_LEN = sector size = 4KB
                continue;
            }
            QMessageBox::critical(this, tr("错误"), m_port.errorMessage(-sub), tr("确定"));
            goto final;
        }

        errCount = 0;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

    QMessageBox::information(this, tr("提示"), tr("写入完成"), tr("确定"));
    if (ui->hexView->data().length() > size) {
        ui->hexView->setData(ui->hexView->data().left(size));
    }

final:
    m_port.closeDevice();
    ui->progressBar->setVisible(false);
    ui->hexView->setBusy(false);
    this->blockActions(false);
}


void MainWindow::on_btn_check_clicked()
{
    if (!ui->hexView->data().length()) {
        QMessageBox::information(this, tr("提示"), tr("缓冲区没有内容"), tr("确定"));
        return;
    }

    int size, step = CH341_MAX_BUF_LEN, //一次读取 4KB
       len = ui->hexView->data().length();
    if (!(size = this->chipSize())) {return;}
    this->initModel();

    int i = m_port.openDevice();
    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    QByteArray stepData;
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->hexView->setBusy(true);
    this->blockActions(true);

    int errCount = 0;
    i = 0;
    while (i < len) {
        if (len - i < step) {
            step = len - i;
        }
        stepData = m_port.read(step, i);
        if (stepData.length() != step) {
            ++errCount;
            if (errCount < 5) {continue;}
            QMessageBox::critical(this, tr("错误"), m_port.errorMessage(Ch341Interface::ReadError), tr("确定"));
            goto final;
        }

        if (stepData != ui->hexView->data().mid(i, step)) {
            QMessageBox::critical(this, tr("提示"), tr("校验内容不一致"), tr("确定"));
            goto final;
        }

        errCount = 0;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

    //校验多出的空间是否为空
    while (i < size) {
        if (size - i < step) {
            step = size - i;
        }
        stepData = m_port.read(step, i);
        if (stepData.length() != step) {
            ++errCount;
            if (errCount < 5) {continue;}
            QMessageBox::critical(this, tr("错误"), m_port.errorMessage(Ch341Interface::ReadError), tr("确定"));
            goto final;
        }

        len = step;
        while (len--) {
            if ((uchar)stepData.at(len) != 0xFF) {
                QMessageBox::critical(this, tr("提示"), tr("校验内容不一致"), tr("确定"));
                goto final;
            }
        }

        errCount = 0;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

    QMessageBox::information(this, tr("提示"), tr("校验完成，内容一致"), tr("确定"));

final:
    m_port.closeDevice();
    ui->progressBar->setVisible(false);
    ui->hexView->setBusy(false);
    this->blockActions(false);
}


void MainWindow::onMemTypeChanged(int index)
{
    ui->combo_model->clear();
    ui->combo_model->addItems(chipModelLists.at(index));
}


void MainWindow::onPortChanged(int index)
{
    m_port.setPort(m_portList.at(index));
}


void MainWindow::on_action_select_zh_CN_triggered()
{
    QApplication::removeTranslator(&m_translator);
    ui->retranslateUi(this);
}


void MainWindow::on_action_select_en_triggered()
{
    QApplication::installTranslator(&m_translator);
    ui->retranslateUi(this);
}


void MainWindow::on_action_cropping_triggered()
{
    int len = ui->hexView->data().length();
    if (!len) {
        QMessageBox::information(this, tr("提示"), tr("缓冲区没有数据，无法裁剪"), tr("确定"));
        return;
    }
    if (nullptr == m_croppingDialog) {
        m_croppingDialog = new CroppingDialog(this);
        connect(m_croppingDialog, SIGNAL(chop(char)), this, SLOT(onChopRequest(char)));
        connect(m_croppingDialog, SIGNAL(cropping(int, int)), this, SLOT(onCroppingRequest(int, int)));
    }

    m_croppingDialog->setStartIndex(0);
    m_croppingDialog->setEndIndex(len - 1);
    m_croppingDialog->setMaximum(len - 1);
    m_croppingDialog->show();
}

void MainWindow::onChopRequest(char ch)
{
    int i = ui->hexView->data().length() - 1;
    while (i >= 0 && ui->hexView->data().at(i) == ch) {
        --i;
    }

    m_croppingDialog->setEndIndex(i);
}

void MainWindow::onCroppingRequest(int start, int end)
{
    if (end < start) {
        ui->hexView->setData(QByteArray());
    }else {
        ui->hexView->setData(ui->hexView->data().mid(start, end - start + 1));
    }
}


void MainWindow::onModelChanged(int index)
{
    if (ui->combo_memType->currentIndex() == 1) { //24 EEPROM
        int size;
        switch (index) {
        case 0:
            ui->label_size->setText(tr("容量：(未知)"));
            break;
        case 1: //24C01
        case 2: //24C02
        case 3: //24C04
            size = 128 * (1 << (index - 1));
            ui->label_size->setText(tr("容量：%1Bytes").arg(size));
            break;
        default:
            size = 1 << (index - 4);
            ui->label_size->setText(tr("容量：%1KBytes").arg(size));
            break;
        };
    }
}

