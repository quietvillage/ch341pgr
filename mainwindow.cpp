#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chipdata.h"

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->on_btn_flush_clicked();
    this->onMemTypeChanged(0);
    m_translator.load(":translations/ch341pgr_en.qm");

    ui->centralwidget->setLayout(ui->hLayout);
    ui->progressBar->setParent(ui->plainTextEdit);
    ui->progressBar->setVisible(false);

    connect(ui->btn_open, SIGNAL(clicked()), this, SLOT(on_action_open_triggered()));
    connect(ui->btn_save, SIGNAL(clicked()), this, SLOT(on_action_save_triggered()));
    connect(ui->combo_memType, SIGNAL(currentIndexChanged(int)), this, SLOT(onMemTypeChanged(int)));
    connect(ui->combo_ports, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(onPortChanged(const QString &)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    ui->progressBar->move((ui->plainTextEdit->width() - ui->progressBar->width()) >> 1,
                          (ui->plainTextEdit->height() - ui->progressBar->height()) >> 1);

    if (e != nullptr) {
        e->ignore();
    }
}

void MainWindow::onloaded()
{
    this->resizeEvent(nullptr);
}

void MainWindow::displayBufferData()
{
    if (!m_buffer.length()) {
        ui->plainTextEdit->clear();
        return;
    }

    uint i = 0, len = m_buffer.length();
    QString text = "00000000:", ascii;

    for (; i < len; ++i) {
        if (i && !(i & 0x0F)) {
            text += " |" + ascii + "|";
            ascii = "";
            text += QString("\n%1:").arg(i, 8, 16, QLatin1Char('0'));
        }

        text += QString(" %1").arg((uchar)m_buffer.at(i), 2, 16, QLatin1Char('0'));
        if ((uchar)m_buffer.at(i) < ' '
           || ((uchar)m_buffer.at(i) >= 0x7F && (uchar)m_buffer.at(i) <= 0xA0)
           || (uchar)m_buffer.at(i) == 0xAD)
        {
            ascii += '\x01';
        } else {
            ascii += m_buffer.at(i);
        }
    }

    if ((len = ascii.length())) {
        text += QString("%1 |%2|").arg(QString((16 - len) * 3, ' '), ascii);
    }

    ui->plainTextEdit->setPlainText(text);
}

void MainWindow::on_action_open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"), "" == m_file? QDir::homePath() : m_file, tr("bin文件(*.bin *.rom);;全部文件(*)"));
    if ("" == fileName) {return;}

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开文件“%1”").arg(fileName), tr("确定"));
        return;
    }

    m_buffer = file.readAll();
    file.close();
    this->displayBufferData();
    m_file = fileName;
    ui->statusbar->showMessage(tr("已选择文件“%1”").arg(m_file), 3000);
}


void MainWindow::on_action_save_triggered()
{
    if (!m_buffer.length()) {
        QMessageBox::information(this, tr("提示"), tr("缓冲区没有内容"), tr("确定"));
        return;
    }

    QFileInfo info(m_file);
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存文件"), (info.path() == "" ? QDir::homePath() : info.path()) + tr("/新建文件"), tr("bin文件(*.bin *.rom);;全部文件(*)"));
    if ("" == fileName) {return;}

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开文件“%1”").arg(fileName), tr("确定"));
        return;
    }

    file.write(m_buffer);
    file.close();
    QMessageBox::information(this, tr("提示"), tr("保存完成"), tr("确定"));
}


void MainWindow::on_action_quit_triggered()
{
    this->close();
}


void MainWindow::on_btn_flush_clicked()
{
    QStringList portList;
    QDir dir("/dev");
    portList = dir.entryList(QDir::System);
    for (int i = portList.size() - 1; i >= 0; --i ) {
        if ("ch34x_pis" != portList.at(i).left(9)) {
            portList.removeAt(i);
        }
    }

    ui->combo_ports->clear();
    if (portList.size()) {
        ui->combo_ports->addItems(portList);
    }

    m_port.setPortName(ui->combo_ports->currentText());
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
                ui->label_size->setText(tr("容量： %2KBytes").arg(1 << ((uchar)info.at(2) - 0x11 + 7)));
            }else {
                ui->label_size->setText(tr("容量： %2MBytes").arg(1 << ((uchar)info.at(2) - 0x14)));
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

void MainWindow::on_btn_reset_clicked()
{
    if (!m_port.openDevice()) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    if (!ui->combo_memType->currentIndex()) {
        m_port.spiReset();
    }

    m_port.closeDevice();
    ui->statusbar->showMessage(tr("复位完成"), 2000);
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
    ui->btn_reset->setEnabled(block);
    ui->btn_open->setEnabled(block);
    ui->action_open->setEnabled(block);
}

void MainWindow::on_btn_checkEmpty_clicked()
{
    int size, step = CH341_DEFAULT_BUF_LEN, j;
    if (!(size = this->chipSize())) {return;}

    this->initModel();
    int i = m_port.openDevice();
    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    if (ui->combo_memType->currentIndex()) {
        m_port.i2cCurrentByte();
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
    int size, step = CH341_DEFAULT_BUF_LEN, len = m_buffer.length();
    if (!(size = this->chipSize())) {return;}
    this->initModel();

    int i = m_port.openDevice();
    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    if (ui->combo_memType->currentIndex()) {
        m_port.i2cCurrentByte();
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
            m_buffer = m_buffer.left(len);
            goto final;
        }

        errCount = 0;
        m_buffer += stepData;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

    m_buffer = m_buffer.mid(len);
    this->displayBufferData();

final:
    m_port.closeDevice();
    ui->progressBar->setVisible(false);
    this->blockActions(false);
}


void MainWindow::on_btn_write_clicked()
{
    if (!m_buffer.length()) {
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

    if (m_buffer.length() > size) {
        i = QMessageBox::question(this, tr("确认"), tr("缓冲区长度已超过存储器容量，超过部分将被直接丢弃，是否继续"), tr("否"), tr("是"));
        if (0 == i) {return;}
    }else {
        size = m_buffer.length();
    }

    if (ui->combo_memType->currentIndex()) {
        m_port.i2cCurrentByte();
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    this->blockActions(true);

    int errCount = 0;
    i = 0;
    while (i < size) {
        if (step > size - i) {
            step = size - i;
        }

        sub = m_port.write(m_buffer.mid(i, step), i);
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
    if (m_buffer.length() > size) {
        m_buffer = m_buffer.left(size);
        this->displayBufferData();
    }

final:
    m_port.closeDevice();
    ui->progressBar->setVisible(false);
    this->blockActions(false);
}


void MainWindow::on_btn_check_clicked()
{
    if (!m_buffer.length()) {
        QMessageBox::information(this, tr("提示"), tr("缓冲区没有内容"), tr("确定"));
        return;
    }

    m_port.setPortName(ui->combo_ports->currentText());
    int size, step = CH341_DEFAULT_BUF_LEN, len = m_buffer.length();
    if (!(size = this->chipSize())) {return;}
    this->initModel();

    int i = m_port.openDevice();
    if (!i) {
        QMessageBox::critical(this, tr("错误"), m_port.errorMessage(), tr("确定"));
        return;
    }

    if (ui->combo_memType->currentIndex()) {
        m_port.i2cCurrentByte();
    }

    QByteArray stepData;
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
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

        if (stepData != m_buffer.mid(i, step)) {
            QMessageBox::critical(this, tr("提示"), tr("校验内容不一致"), tr("确定"));
            goto final;
        }

        errCount = 0;
        i += step;
        ui->progressBar->setValue(i * 100 / size);
        QApplication::processEvents();
    }

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
    this->blockActions(false);
}


void MainWindow::onMemTypeChanged(int index)
{
    ui->combo_model->clear();
    ui->combo_model->addItems(chipModelLists.at(index));
}


void MainWindow::onPortChanged(const QString &port)
{
    m_port.setPortName(port);
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

