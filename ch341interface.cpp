#include "ch341interface.h"
#include "chipdata.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <QFileInfo>

static
const uchar msbLsbSwappedTable[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
    0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,// 0XH
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,// 1XH
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
    0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,// 2XH
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
    0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,// 3XH
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,// 4XH
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,// 5XH
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
    0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,// 6XH
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,// 7XH
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
    0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,// 8XH
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
    0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,// 9XH
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,// AXH
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
    0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,// BXH
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
    0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,// CXH
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,// DXH
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
    0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,// EXH
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
    0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

Ch341Interface::Ch341Interface():
    m_fd(0),
    m_error(NoError),
    m_memType(SpiFlash),
    m_modelIndex(0),
    m_vendorIc(0x00),
    m_streamMode(0x81),
    m_spiCS(0x80)
{

}

bool Ch341Interface::openDevice()
{
    QFileInfo info("/dev/" + m_portName);
    if ("" == m_portName || !info.exists()) {
        m_error = DeviceNotFoundError;
        return false;
    }

    if (!(info.isReadable() && info.isWritable())) {
        m_error = PermissionError;
        return false;
    }

    m_fd = open(info.filePath().toLocal8Bit(), O_RDWR);
    if (m_fd < 0) {
         m_error = OpenError;
         return false;
    }

    this->vendorId(); //初始化 vendorIc 后才能正常使用

    if (m_vendorIc < 0x20) {
        m_error = UnsupportedOperationError;
        return false;
    }

    m_error = NoError;
    return true;
}

void Ch341Interface::closeDevice()
{
    close(m_fd);
    m_fd = 0;
}

QString Ch341Interface::driverVersion()
{
    char str[128] = {0x00};
    ioctl(m_fd, CH34x_GET_DRIVER_VERSION, str);
    return QString::fromLocal8Bit(str);
}

QByteArray Ch341Interface::vendorId()
{
    char str[4] = {0x00};
    m_vendorIc = 0x00;
    if (ioctl(m_fd, CH34x_CHIP_VERSION, str) >= 0) {
        m_vendorIc = str[0];
    }

    return QByteArray(str, 2);
}

QString Ch341Interface::errorMessage(int code)
{
    QString text;
    if (code < 0) {
        code = m_error;
    }
    switch (code) {
    case DeviceNotFoundError: text = QObject::tr("设备不存在"); break;
    case PermissionError: text = QObject::tr("没有权限或端口忙"); break;
    case OpenError: text = QObject::tr("打开设备出错"); break;
    case ReadError: text = QObject::tr("读取出错"); break;
    case WriteError: text = QObject::tr("写入出错"); break;
    case UnsupportedOperationError: text = QObject::tr("该设备不支持此操作"); break;
    default: break;
    }

    return text;
}

char Ch341Interface::i2cCurrentByte()
{
    QByteArray buf;
    char outBuf[2];
    unsigned long retLen, data[4];
    data[2] = (unsigned long) outBuf;
    data[3] = (unsigned long) &retLen;
    buf.append(CH341A_CMD_I2C_STREAM);
    if (!(m_streamMode & 0x03)) {
        buf.append(2, CH341A_CMD_I2C_STREAM_DELAY_US | 10);
    }

    buf.append(CH341A_CMD_I2C_STREAM_START);
    buf.append((char)CH341A_CMD_I2C_STREAM_OUT | 1);
    buf.append(0xA1);

    buf.append((char)CH341A_CMD_I2C_STREAM_IN | 1);

    buf.append(CH341A_CMD_I2C_STREAM_IN);
    buf.append(CH341A_CMD_I2C_STREAM_STOP);
    buf.append(1, CH341A_CMD_I2C_STREAM_END);

    buf.append(CH341_PACKET_LENGTH - 1);
    buf.append(7, 1);

    data[0] = buf.length();
    data[1] = (unsigned long) buf.data();

    ioctl(m_fd, CH34x_PIPE_WRITE_READ, data);
    usleep(10000);//延时 10ms
    return outBuf[0];
}

QByteArray Ch341Interface::read(uint n, uint addr)
{
    QByteArray data;
    if (I2cEEPROM == m_memType) {
        uchar addrSize = 3;
        if (m_modelIndex <= EEPROM_MODEL_INDEX_24C16) {
            addrSize = 2;
        }

        data = i2cReadPrivate(n, addr, addrSize);
    }else {
        data = spiReadPrivate(n, addr);
    }

    return data;
}

int Ch341Interface::write(const QByteArray &data, uint addr)
{
    int sum;
    if (SpiFlash == m_memType) {
        sum = spiWritePrivate(data, addr);
    }else {
        sum = i2cWritePrivate(data, addr);
    }

    return sum;
}

bool Ch341Interface::ch341writePrivate(const QByteArray &in)
{
    unsigned long len = in.length(),
    data[2] = {
        (unsigned long) &len,
        (unsigned long) in.data()
    };

    if (ioctl(m_fd, CH34x_PIPE_DATA_DOWN, data) < 0 || (int)len != in.length()) {
        m_error = WriteError;
        return false;
    }

    return true;
}


QByteArray Ch341Interface::spiStreamPrivate(const QByteArray &in, bool isRead)
{
    if (m_vendorIc < 0x20 || (m_vendorIc > 0x25 && m_vendorIc < 0x30)) {
        m_error = UnsupportedOperationError;
        return "";
    }

    char outBuf[CH341_MAX_BUF_LEN];
    QByteArray buf;
    if (m_spiCS & 0x80) {
        buf.append(CH341A_CMD_UIO_STREAM);
        switch (m_spiCS & 0x03) {
        case 0x00: buf.append((char)CH341A_CMD_UIO_STREAM_OUT | 0x36); break; //DCK/D3->0, D0 ->0
        case 0x01: buf.append((char)CH341A_CMD_UIO_STREAM_OUT | 0x35); break; //DCK/D3->0, D1 ->0
        case 0x02: buf.append((char)CH341A_CMD_UIO_STREAM_OUT | 0x33); break; //DCK/D3->0, D2 ->0
        default: buf.append((char)CH341A_CMD_UIO_STREAM_OUT | 0x27); break; //DCK/D3->0, D4 ->0
        }

        buf.append(CH341A_CMD_UIO_STREAM_DIRECT | 0x3F);
        buf.append(CH341A_CMD_UIO_STREAM_END);
        buf.append(CH341_PACKET_LENGTH - buf.length() % CH341_PACKET_LENGTH, 0x00);
    }

    int i = buf.length() + CH341_PACKET_LENGTH;
    int j = 0, len = in.length();
    buf.append(CH341A_CMD_SPI_STREAM);

    if (isRead) {
        while (j < 4 && j < len) {
            buf.append(msbLsbSwappedTable[(uchar)in.at(j++)]); //cmd + addr
        }

        if (j < len) {
            buf.resize(buf.length() + len - j);
        }
    }else {
        while (j < len) {
            buf.append(msbLsbSwappedTable[(uchar)in.at(j++)]);
        }
    }

    while (i < buf.length()) {
        if (0x20 == m_vendorIc) {
            buf.insert(i, CH341_PACKET_LENGTH, 0x00);
            i += CH341_PACKET_LENGTH;
        }

        buf.insert(i, CH341A_CMD_SPI_STREAM);
        i += CH341_PACKET_LENGTH;
    }

    buf.append(CH341_PACKET_LENGTH - 1);
    buf.append(7, (len + CH341_PACKET_LENGTH - 1 - 1) / (CH341_PACKET_LENGTH - 1));

    unsigned long retLen, data[4];
    data[0] = buf.length();
    data[1] = (unsigned long) buf.data();
    data[2] = (unsigned long) outBuf;
    data[3] = (unsigned long) &retLen;

    if (ioctl(m_fd, CH34x_PIPE_WRITE_READ, data) < 0) {
        return "";
    }

    buf.clear();
    if (m_spiCS & 0x80) {
        buf.append(CH341A_CMD_UIO_STREAM);
        buf.append((char)CH341A_CMD_UIO_STREAM_OUT | 0x37);
        buf.append(CH341A_CMD_UIO_STREAM_END);

        if (!ch341writePrivate(buf)) {return "";}
    }

    if (isRead) {
        buf.clear();
        if (m_streamMode & 0x80) {
            while (retLen--) {
                buf.prepend(msbLsbSwappedTable[(uchar)outBuf[retLen]]);
            }
        }else {
            buf.append(outBuf, retLen);
        }

        return buf;
    }

    return "\x01"; //区别于错误情况
}


QByteArray Ch341Interface::spiJedecId()
{
    QByteArray buf;
    buf.append(SPI_CMD_JEDEC_ID);
    buf.append(1, 0x00); //addr23-16
    buf.append(1, 0x00); //addr15-8
    buf.append(1, 0x00); //addr7-0
    buf = spiStreamPrivate(buf, OPERATION_SPI_STREAM_READ);
    if (buf.length()) {
        buf.remove(0, 1);
    }else {
        m_error = ReadError;
    }

    return buf;
}

QByteArray Ch341Interface::spiReadPrivate(uint n, uint addr)
{
    QByteArray buf;

    buf.append(SPI_CMD_READ_DATA); //cmd
    buf.append(addr >> 16); //addr23 - 16
    buf.append(addr >> 8); //addr15 - 8
    buf.append(addr); //addr7 - 0
    buf.resize(n + 4); //can be any value

    buf = spiStreamPrivate(buf, OPERATION_SPI_STREAM_READ);
    if (buf.length()) {
        buf.remove(0, 4);
    }else {
        m_error = ReadError;
    }

    return buf;
}

QByteArray Ch341Interface::spiGetStatusPrivate(uchar cmd)
{
    QByteArray buf;
    buf.append(cmd);
    buf.append(1, 0x00);
    buf = spiStreamPrivate(buf, OPERATION_SPI_STREAM_READ);

    if (buf.length() > 1) {
        buf = buf.mid(1, 1);
    }else {
        buf.clear();
    }

    return buf;
}

bool Ch341Interface::spiWriteEnablePrivate()
{
    if (!spiWaitPrivate()) {
        return false;
    }

    QByteArray buf;
    buf.append(SPI_CMD_WRITE_ENABLE);
    return spiStreamPrivate(buf, OPERATION_SPI_STREAM_WRITE).length();
}

bool Ch341Interface::spiWaitPrivate()
{
    QByteArray buf;

    while (1) {
        buf = spiGetStatusPrivate(SPI_CMD_READ_SR);

        if (!buf.length()) {
            return false;
        }

        if (!(buf.at(0) & SPI_STATUS_BUSY)) {
            return true;
        }
    }

    return false;
}

bool Ch341Interface::spiEraseChip()
{
   if (!spiWriteEnablePrivate()) {return false;}
   QByteArray buf;

   buf.append(SPI_CMD_ERASE_CHIP);
   if (!spiStreamPrivate(buf, OPERATION_SPI_STREAM_WRITE).length()) {
       m_error = WriteError;
       return false;
   }

   return spiWaitPrivate();
}

bool Ch341Interface::spiEraseSector(uint addr)
{
    if (!spiWriteEnablePrivate()) {return false;}
    QByteArray buf;
    buf.append(SPI_CMD_ERASE_4KB);
    buf.append(addr >> 16);
    buf.append(addr >> 8);
    buf.append(addr);

    if (!spiStreamPrivate(buf, OPERATION_SPI_STREAM_WRITE).length()) {
        m_error = WriteError;
        return false;
    }

    return spiWaitPrivate();
}

bool Ch341Interface::spiReset()
{
    if (!spiWaitPrivate()) {return false;}

    QByteArray buf;
    buf.append(SPI_CMD_RESET_ENABLE);

    if (!spiStreamPrivate(buf, OPERATION_SPI_STREAM_WRITE).length()) {return false;}

    sleep(1);

    buf[0] = SPI_CMD_RESET;
    return spiStreamPrivate(buf, OPERATION_SPI_STREAM_WRITE).length();
}

bool Ch341Interface::spiPageProgramPrivate(const QByteArray &in, uint addr)
{
    if (!spiWriteEnablePrivate()) {return false;}

    QByteArray buf;
    buf.append(SPI_CMD_PAGE_PROGRAM);
    buf.append(addr >> 16);
    buf.append(addr >> 8);
    buf.append(addr);
    buf += in;

    if (!spiStreamPrivate(buf, OPERATION_SPI_STREAM_WRITE).length()) {
        m_error = WriteError;
        return false;
    }

    return spiWaitPrivate();
}

int Ch341Interface::spiWritePrivate(const QByteArray &in, uint addr)
{
    int i = 0, sum = 0,
    step = SPI_MAX_PAGE_PRAGRAM_SIZE - addr % SPI_MAX_PAGE_PRAGRAM_SIZE,
    len = in.length();

    if (len < step) {
        step = len;
    }

    if (!spiPageProgramPrivate(in.mid(i, step), addr)) {
        m_error = WriteError;
        return -WriteError;
    }
    sum += step;
    len -= step;
    i += step;
    addr += step;
    step = SPI_MAX_PAGE_PRAGRAM_SIZE;

    while (len) {
        if (len < step) {
            step = len;
        }

        if (!spiPageProgramPrivate(in.mid(i, step), addr)) {
            m_error = WriteError;
            return -WriteError;
        }

        sum += step;
        i += step;
        addr += step;
        len -= step;
    }

    return sum;
}


QByteArray Ch341Interface::i2cReadPrivate(uint n, uint addr, uchar addrSize)
{
    if (m_vendorIc < 0x20) {
        m_error = UnsupportedOperationError;
        return "";
    }

    char outBuf[CH341_MAX_BUF_LEN];
    QByteArray inBuf;
    unsigned long retLen, data[4];
    data[2] = (unsigned long) outBuf;
    data[3] = (unsigned long) &retLen;

    inBuf.append(CH341A_CMD_I2C_STREAM);

    if (!(m_streamMode & 0x03)) {
        inBuf.append(2, CH341A_CMD_I2C_STREAM_DELAY_US | 10);
    }

    inBuf.append(CH341A_CMD_I2C_STREAM_START);
    inBuf.append(CH341A_CMD_I2C_STREAM_OUT | addrSize);
    inBuf.append(0xA0 | ((addr >> ((addrSize - 1) * 8 - 1)) & 0x0E));
    int i = addrSize - 2;
    while (i > 0) {
        inBuf.append(addr >> (i * 8));
        i--;
    }
    inBuf.append(addr);

    inBuf.append(CH341A_CMD_I2C_STREAM_START);
    inBuf.append((char) CH341A_CMD_I2C_STREAM_OUT | 1);
    inBuf.append(0xA0 | ((addr >> ((addrSize - 1) * 8 - 1)) & 0x0E) | 0x01); //器件地址（读）

    int step = CH341_PACKET_LENGTH - 1;
    i = n;
    while (i > 0) {
        if (i < step) {
            step = i;
        }

        inBuf.append(CH341A_CMD_I2C_STREAM_IN | step);
        if (step >= CH341_PACKET_LENGTH - 1) {
            inBuf.append(1, CH341A_CMD_I2C_STREAM_END);
            inBuf.append(CH341_PACKET_LENGTH - inBuf.length() % CH341_PACKET_LENGTH, 0x00);
            inBuf.append(CH341A_CMD_I2C_STREAM);
        }

        i -= step;
    }

    inBuf.append(CH341A_CMD_I2C_STREAM_IN);
    inBuf.append(CH341A_CMD_I2C_STREAM_STOP);
    inBuf.append(1, CH341A_CMD_I2C_STREAM_END);

    inBuf.append(CH341_PACKET_LENGTH - 1); //[index = length - 8] 一次读取的量

    //[index = length - 4] //读取次数
    inBuf.append(7, (n + CH341_PACKET_LENGTH - 1 - 1) / (CH341_PACKET_LENGTH - 1));

    data[0] = inBuf.length();
    data[1] = (unsigned long) inBuf.data();

    if (ioctl(m_fd, CH34x_PIPE_WRITE_READ, data) < 0) {
        m_error = ReadError;
        return "";
    }

    return QByteArray(outBuf, retLen);
}


int Ch341Interface::i2cWritePrivate(const QByteArray &in, uint addr)
{
    if (m_vendorIc < 0x20) {
        m_error = UnsupportedOperationError;
        return -UnsupportedOperationError;
    }

    int i, pageSize, len = in.length();
    uchar addrSize = 3;
    if (m_modelIndex <= EEPROM_MODEL_INDEX_24C16) {
        addrSize = 2;
    }

    if(m_modelIndex <= EEPROM_MODEL_INDEX_24C01) {
        pageSize = 4;
    }else if(m_modelIndex <= EEPROM_MODEL_INDEX_24C02) {
        pageSize = 8;
    }else if (m_modelIndex <= EEPROM_MODEL_INDEX_24C16) {
        pageSize = 16;
    }else if (m_modelIndex <= EEPROM_MODEL_INDEX_24C64) {
        pageSize = 32;
    }else if (m_modelIndex <= EEPROM_MODEL_INDEX_24C256) {
        pageSize = 64;
    }else if (m_modelIndex <= EEPROM_MODEL_INDEX_24C512) {
        pageSize = 128;
    }else {
        pageSize = 256;
    }


    //第一页
    i = pageSize - addr % pageSize;
    if (len < i) {
        i = len;
    }

    if (!i2cPageProgramPrivate(in.left(i), addr, addrSize)) {
        m_error = WriteError;
        return -WriteError;
    }
    addr += i;
    len -= i;

    while (len) {
        usleep(10000); //延时 10ms
        if (len < pageSize) {
            pageSize = len;
        }

        if (!i2cPageProgramPrivate(in.mid(i, pageSize), addr, addrSize)) {
            m_error = WriteError;
            return -WriteError;
        }

        addr += pageSize;
        i += pageSize;
        len -= pageSize;
    }

    return in.length();
}

bool Ch341Interface::i2cPageProgramPrivate(const QByteArray &in, uint addr, uchar addrSize)
{
    QByteArray buf;
    int i, len;
    buf.append(CH341A_CMD_I2C_STREAM);
    if (!(m_streamMode & 0x03)) {
        buf.append(2, CH341A_CMD_I2C_STREAM_DELAY_US | 10);
    }
    buf.append(CH341A_CMD_I2C_STREAM_START);
    len = CH341_PACKET_LENGTH - buf.length() - 2;
    if (len > in.length() + addrSize) {
        len = in.length() + addrSize;
    }
    buf.append(CH341A_CMD_I2C_STREAM_OUT | len);

    buf.append(0xA0 | (addr >> ((addrSize - 1) * 8 - 1) & 0x0E));
    i = addrSize - 2;
    while (i > 0) {
        buf.append(addr >> (i * 8));
        i--;
    }
    buf.append(addr);
    buf += in;

    i = CH341_PACKET_LENGTH - 1;
    len = CH341_PACKET_LENGTH - 3;

    while (i < buf.size() + 1) {
        buf.insert(i, (char) CH341A_CMD_I2C_STREAM_END);
        buf.insert(i + 1, (char) CH341A_CMD_I2C_STREAM);
        if (buf.size() - i - 2 < len) {
            len = buf.size() - i - 2;
        }
        buf.insert(i + 2, CH341A_CMD_I2C_STREAM_OUT | len);
        i += CH341_PACKET_LENGTH;
    }

    buf.append(CH341A_CMD_I2C_STREAM_STOP);
    buf.append(1, CH341A_CMD_I2C_STREAM_END);

    return ch341writePrivate(buf);
}

