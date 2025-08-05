#include "ch341interface.h"
#include "chipdata.h"

#include <unistd.h>
#include <QObject>

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

Ch341Interface::Ch341Interface()
    : m_error(NoError)
    , m_memType(SpiFlash)
    , m_modelIndex(0)
    , m_chipVersion(0x00)
    , m_spiCS(0x00) //默认 CS 线 -> D0
    , m_bulkInEndpointAddr(0)
    , m_bulkOutEndpointAddr(0)
{

}

bool Ch341Interface::openDevice()
{
    m_error = OpenError;
    m_devHandle = nullptr;

    int ret, i, count;
    ret = libusb_init(nullptr);
    if (ret != LIBUSB_SUCCESS) {return false;}

    struct libusb_device **devices;
    struct libusb_device_descriptor devDescriptor;
    count = libusb_get_device_list(nullptr, &devices);

    for (i = 0; i < count; ++i) {
        if (libusb_get_bus_number(devices[i]) != m_port.first ||
                libusb_get_port_number(devices[i]) != m_port.second)
        {
            continue;
        }

        ret = libusb_get_device_descriptor(devices[i], &devDescriptor);
        if (ret != LIBUSB_SUCCESS) {
            goto _errExit;
        }

        if (CH341_VENDOR_ID != devDescriptor.idVendor ||
            CH341_PRODUCT_ID != devDescriptor.idProduct)
        {
            m_error = DeviceNotFoundError;
            goto _errExit;
        }

        ret = libusb_open(devices[i], &m_devHandle);
        if (ret != LIBUSB_SUCCESS) {
            if (LIBUSB_ERROR_ACCESS == ret) {
                m_error = PermissionError;
            }
            goto _errExit;
        }

        if (libusb_kernel_driver_active(m_devHandle, 0) == 1) {
            if (libusb_detach_kernel_driver(m_devHandle, 0) != 0) {//卸载内核驱动
                goto _errCloseDevice;
            }
        }

        int configs;
        ret = libusb_get_configuration(m_devHandle, &configs);
        if (ret != LIBUSB_SUCCESS) {
            goto _errReattachDriver;
        }

        libusb_config_descriptor *configDesc;
        const libusb_endpoint_descriptor *endpDesc;
        ret = libusb_get_config_descriptor(devices[i], 0, &configDesc);
        if (ret != LIBUSB_SUCCESS) {
            goto _errReattachDriver;
        }

        for (int j = 0; j < configDesc->interface[0].altsetting[0].bNumEndpoints; ++j) {
            endpDesc = &configDesc->interface[0].altsetting[0].endpoint[j];
            if ((endpDesc->bEndpointAddress & LIBUSB_ENDPOINT_IN) &&
                (endpDesc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
            {
                m_bulkInEndpointAddr = endpDesc->bEndpointAddress; // 0x82
            }

            if (!(endpDesc->bEndpointAddress & LIBUSB_ENDPOINT_IN) &&
                (endpDesc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
            {
                m_bulkOutEndpointAddr = endpDesc->bEndpointAddress; //0x02
            }
        }

        libusb_free_config_descriptor(configDesc);
        ret = libusb_claim_interface(m_devHandle, 0);
        if (ret != LIBUSB_SUCCESS) {
            goto _errReattachDriver;
        }

        break;
    }

    if (i >= count) {
        m_error = DeviceNotFoundError;
        goto _errExit;
    }

    if (getChipVersion()) {//获取 chipVersion 后才能正常使用
        if (SpiFlash == m_memType && m_modelIndex >= SPI_FLASH_MODEL_INDEX_25Q256 &&
            !this->spiEnter4ByteAddrModePrivate())
        {//进入4字节地址模式
            goto _err;
        }
        
        libusb_free_device_list(devices, 1);
        m_error = NoError;
        return true;
    }
    
    
    //err
_err:
    libusb_release_interface(m_devHandle, 0);
_errReattachDriver:
    libusb_attach_kernel_driver(m_devHandle, 0);
_errCloseDevice:
    libusb_close(m_devHandle);
_errExit:
    libusb_free_device_list(devices, 1);
    libusb_exit(nullptr); 
    m_devHandle = nullptr;
    return false;
}

void Ch341Interface::closeDevice()
{
    if (m_devHandle) {
        libusb_release_interface(m_devHandle, 0);
        libusb_attach_kernel_driver(m_devHandle, 0);
        libusb_close(m_devHandle);
        libusb_exit(nullptr);
        m_devHandle = nullptr;
    }
}


bool Ch341Interface::getChipVersion()
{
    int ret;
    m_chipVersion = 0x00;

    uchar data[2] = {0x00};
    ret = libusb_control_transfer(m_devHandle,
              LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
              CH341_REQ_CHIP_VERSION, 0x0000, 0x0000, data, 2, 1000);
    //此处 ret 等于 2
    if (ret != 2) {return false;}
    m_chipVersion = data[0];

    return true;
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
    case OverflowError: text = QObject::tr("缓存溢出"); break;
    default: break;
    }

    return text;
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

bool Ch341Interface::spiStreamStartPrivate()
{
    QByteArray buf;
    int ret;

    //driving /CS and CLK(D3) low
    buf.append(CH341_STREAM_TYPE_UIO);
    switch (m_spiCS & 0x03) {
    case 0x00: buf.append((char)CH341_UIO_STREAM_CMD_OUT | 0x36); break; //DCK/D3->0, CS/D0 ->0
    case 0x01: buf.append((char)CH341_UIO_STREAM_CMD_OUT | 0x35); break; //DCK/D3->0, CS/D1 ->0
    case 0x02: buf.append((char)CH341_UIO_STREAM_CMD_OUT | 0x33); break; //DCK/D3->0, CS/D2 ->0
    case 0x03: buf.append((char)CH341_UIO_STREAM_CMD_OUT | 0x27); break; //DCK/D3->0, CS/D4 ->0
    }

    buf.append(CH341_UIO_STREAM_CMD_DIRECT | 0x3F); //D5~D0
    buf.append(CH341_UIO_STREAM_CMD_END);

    ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
            (uchar *)buf.data(), buf.length(), nullptr, 1000);
    return (LIBUSB_SUCCESS == ret);
}

bool Ch341Interface::spiStreamStopPrivate()
{
    QByteArray buf;
    int ret;

    buf.append(CH341_STREAM_TYPE_UIO);
    buf.append((char)CH341_UIO_STREAM_CMD_OUT | 0x37); //driving /CS(D0、D1、D2、D4) high
    buf.append(CH341_UIO_STREAM_CMD_END);

    ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
            (uchar *)buf.data(), buf.length(), nullptr, 1000);

    return (LIBUSB_SUCCESS == ret);
}

//spi 指令流，读取及配置寄存器、擦除等操作, 数据长度小于一个包
QByteArray Ch341Interface::spiCmdStreamPrivate(const QByteArray &in, int readLen)
{
    if (m_chipVersion < 0x20 || (m_chipVersion > 0x25 && m_chipVersion < 0x30)) {
        m_error = UnsupportedOperationError;
        return "";
    }

    if (readLen >= CH341_PACKET_LENGTH || in.length() >= CH341_PACKET_LENGTH) {
        m_error = OverflowError;
        return "";
    }

    uchar outBuf[CH341_PACKET_LENGTH];
    uchar inBuf[CH341_PACKET_LENGTH];
    int i, packLen, len, ret;

    inBuf[0] = CH341_STREAM_TYPE_SPI;

    i = 0;
    while (i < in.length()) {
        inBuf[i + 1] = msbLsbSwappedTable[(uchar)in.at(i)];
        ++i;
    }

    packLen = i + 1;
    if ((int)readLen >= packLen) {
        packLen = readLen + 1;
    }

    //spi start
    if (!spiStreamStartPrivate()) {
        return "";
    }


    ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
            inBuf, packLen, nullptr, 1000);
    if (ret != LIBUSB_SUCCESS) {return "";}

    ret = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
            outBuf, packLen - 1, &len, 1000);
    if (ret != LIBUSB_SUCCESS) {return "";}

    //spi stop
    if (!spiStreamStopPrivate()) {
        return "";
    }

    if (len < (int)readLen) {
        return "";
    }

    if (readLen) {
        len = readLen;
        //spi数据总是低位先发
        while (--len >= 0) {
            outBuf[len] = msbLsbSwappedTable[outBuf[len]];
        }

        return QByteArray((char *) outBuf, readLen);
    }

    return "\x01"; //区别于错误情况
}

QByteArray Ch341Interface::spiJedecId()
{
    QByteArray buf;
    buf.append(SPI_CMD_JEDEC_ID);

    buf = this->spiCmdStreamPrivate(buf, 4);

    if (buf.length() > 1) {
        return buf.mid(1);
    }

    return "";
}


QByteArray Ch341Interface::spiReadPrivate(uint n, uint addr)
{
    if (m_chipVersion < 0x20 || (m_chipVersion > 0x25 && m_chipVersion < 0x30)) {
        m_error = UnsupportedOperationError;
        return "";
    }

    if (n > CH341_MAX_BUF_LEN) {
        m_error = OverflowError;
        return "";
    }

    uchar outBuf[CH341_MAX_BUF_LEN + 8]; //预留8个headLen
    uchar inBuf[CH341_PACKET_LENGTH];

    int i, ret, packLen, retLen, sum, headLen;
    headLen = (m_modelIndex >= SPI_FLASH_MODEL_INDEX_25Q256) ? 5 : 4;

    inBuf[0] = CH341_STREAM_TYPE_SPI;
    inBuf[1] = msbLsbSwappedTable[SPI_CMD_READ_DATA]; //cmd
    if (m_modelIndex >= SPI_FLASH_MODEL_INDEX_25Q256) {
        inBuf[2] = msbLsbSwappedTable[(addr >> 24) & 0xFF];  //addr31 - 24
        inBuf[3] = msbLsbSwappedTable[(addr >> 16) & 0xFF]; //addr23 - 16
        inBuf[4] = msbLsbSwappedTable[(addr >> 8) & 0xFF]; //addr15 - 8
        inBuf[5] = msbLsbSwappedTable[addr & 0xFF]; //addr7 - 0
    }else {
        inBuf[2] = msbLsbSwappedTable[(addr >> 16) & 0xFF]; //addr23 - 16
        inBuf[3] = msbLsbSwappedTable[(addr >> 8) & 0xFF]; //addr15 - 8
        inBuf[4] = msbLsbSwappedTable[addr & 0xFF]; //addr7 - 0
    }
    
    //发起起始条件，拉低 CS、CLK
    if (!spiStreamStartPrivate()) {
        return "";
    }

    sum = n + headLen; //total read
    packLen = CH341_PACKET_LENGTH;
    i = 0;
    while (i < sum) {
        if (packLen > sum - i + 1) {
            packLen = sum - i + 1;
        }

        ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  inBuf, packLen, nullptr, 1000);
        if (ret != LIBUSB_SUCCESS) {return "";}

        ret = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
                  &outBuf[i], packLen - 1, &retLen, 1000);
        if (ret != LIBUSB_SUCCESS) {return "";}
        i += retLen;
    }


    //结束操作，拉高 CS
    if (!spiStreamStopPrivate()) {
        return "";
    }

    i = sum - 1;
    while (i >= headLen) {
        outBuf[i] = msbLsbSwappedTable[outBuf[i]];
        --i;
    }

    return QByteArray((char *)outBuf + headLen, n);
}


inline QByteArray Ch341Interface::spiGetStatusPrivate(uchar cmd)
{
    QByteArray buf;
    buf.append(cmd);

    buf = this->spiCmdStreamPrivate(buf, 2);

    if (buf.length() > 1) {
        return buf.mid(1);
    }

    return "";
}

bool Ch341Interface::spiWriteEnablePrivate()
{
    if (!spiWaitPrivate()) {return false;}

    QByteArray buf;
    buf.append(SPI_CMD_WRITE_ENABLE);

    return spiCmdStreamPrivate(buf, 0).length();
}

bool Ch341Interface::spiWaitPrivate()
{
    QByteArray buf;
    int errCount = 0;

    while (1) {
        buf = spiGetStatusPrivate(SPI_CMD_READ_SR);

        if (!buf.length()) {
            ++errCount;
            if (errCount > 5) {
                return false;
            }
        }else {
            errCount = 0;
            if (!(buf.at(0) & SPI_STATUS_BUSY)) {
                return true;
            }
        }
    }

    return false;
}

bool Ch341Interface::spiEnter4ByteAddrModePrivate()
{
    QByteArray buf;
    buf.append(SPI_CMD_ENTER_4BYTE_ADDR_MODE);
    
    return spiCmdStreamPrivate(buf, 0).length();
}

bool Ch341Interface::spiExit4ByteAddrModePrivate()
{
    QByteArray buf;
    buf.append(SPI_CMD_EXIT_4BYTE_ADDR_MODE);
    
    return spiCmdStreamPrivate(buf, 0).length();
}

bool Ch341Interface::spiEraseChip()
{
   if (!spiWriteEnablePrivate()) {return false;}
   QByteArray buf;
   buf.append(SPI_CMD_ERASE_CHIP);

   if (!spiCmdStreamPrivate(buf, 0).length()) {
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
    if (m_modelIndex >= SPI_FLASH_MODEL_INDEX_25Q256) {
        buf.append(addr >> 24);
        buf.append(addr >> 16);
        buf.append(addr >> 8);
        buf.append(addr);
    }else {
        buf.append(addr >> 16);
        buf.append(addr >> 8);
        buf.append(addr);
    }
    

    if (!spiCmdStreamPrivate(buf, 0).length()) {
        m_error = WriteError;
        return false;
    }

    return spiWaitPrivate();
}

int Ch341Interface::spiWritePrivate(const QByteArray &in, uint addr)
{
    if (m_chipVersion< 0x20 || (m_chipVersion > 0x25 && m_chipVersion < 0x30)) {
        m_error = UnsupportedOperationError;
        return -UnsupportedOperationError;
    }

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

bool Ch341Interface::spiPageProgramPrivate(const QByteArray &in, uint addr)
{
    uchar inBuf[CH341_PACKET_LENGTH];
    uchar outBuf[CH341_PACKET_LENGTH];
    uchar zeroPacket[CH341_PACKET_LENGTH] = {0x00};

    int i, dataIndex, dataLen, ret, packLen;

    if (!spiWriteEnablePrivate()) {return false;}

    //发起起始条件，拉低 CS、CLK
    if (!spiStreamStartPrivate()) {
        return false;
    }

    inBuf[0] =  CH341_STREAM_TYPE_SPI;
    inBuf[1] = msbLsbSwappedTable[(uchar)SPI_CMD_PAGE_PROGRAM];
    if (m_modelIndex >= SPI_FLASH_MODEL_INDEX_25Q256) {
        inBuf[2] = msbLsbSwappedTable[(addr >> 24) & 0xFF];
        inBuf[3] = msbLsbSwappedTable[(addr >> 16) & 0xFF];
        inBuf[4] = msbLsbSwappedTable[(addr >> 8) & 0xFF];
        inBuf[5] = msbLsbSwappedTable[addr & 0xFF];
        i = 6; //包已被占用长度
    }else {
        inBuf[2] = msbLsbSwappedTable[(addr >> 16) & 0xFF];
        inBuf[3] = msbLsbSwappedTable[(addr >> 8) & 0xFF];
        inBuf[4] = msbLsbSwappedTable[addr & 0xFF];
        i = 5;
    }
    

    dataLen = in.length();
    dataIndex = 0;
    while (dataIndex < dataLen) {
        while (i < CH341_PACKET_LENGTH && dataIndex < dataLen) {
            inBuf[i++] = msbLsbSwappedTable[(uchar)in.at(dataIndex++)];
        }

        packLen = i; //发送包长度
        ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  inBuf, packLen, nullptr, 1000);
        if (ret != LIBUSB_SUCCESS) {return false;}

        if (0x20 == m_chipVersion) {
            ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                      zeroPacket, CH341_PACKET_LENGTH, nullptr, 1000);
            if (ret != LIBUSB_SUCCESS) {return false;}
        }

        //清空缓冲区
        libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
            outBuf, packLen - 1, nullptr, 1000);
        
        i = 1; //开始下一个数据包
    }

    //结束操作，拉高 CS
    if (!spiStreamStopPrivate()) {
        return false;
    }

    return spiWaitPrivate();
}

QByteArray Ch341Interface::i2cReadPrivate(uint n, uint addr, uchar addrSize)
{
    if (m_chipVersion < 0x20) {
        m_error = UnsupportedOperationError;
        return "";
    }

    if (n > CH341_MAX_BUF_LEN) {
        m_error = OverflowError;
        return "";
    }

    uchar outBuf[CH341_MAX_BUF_LEN];
    QByteArray inBuf;

    int i, j, ret, retLen, step, remain;

    inBuf.append(CH341_STREAM_TYPE_I2C);

    inBuf.append(CH341_I2C_STREAM_CMD_START);
    inBuf.append(CH341_I2C_STREAM_CMD_OUT | addrSize);

    inBuf.append(0xA0 | ((addr >> ((addrSize - 1) * 8 - 1)) & 0x0E));
    i = addrSize - 2;
    while (i > 0) {
        inBuf.append(addr >> (i * 8));
        --i;
    }
    inBuf.append(addr);

    inBuf.append(CH341_I2C_STREAM_CMD_START);
    inBuf.append((char) CH341_I2C_STREAM_CMD_OUT | 1);
    inBuf.append(0xA0 | ((addr >> ((addrSize - 1) * 8 - 1)) & 0x0E) | 0x01); //器件地址（读）

    step = CH341_PACKET_LENGTH;
    i = 0;
    while (i < (int)n) {
        if (step > (int)n - i) {
            step = (int)n - i;
        }

        inBuf.append(CH341_I2C_STREAM_CMD_IN | step);
        inBuf.append(1, CH341_I2C_STREAM_CMD_END);

        ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  (uchar *)inBuf.data(), inBuf.length(), nullptr, 1000);
        if (ret != LIBUSB_SUCCESS) {return "";}

        remain = step;
        for (j = 0; j < 5; ++j) {
            usleep(1000); //等待 1ms，以便数据就位(100kHz)

            ret = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
                      &outBuf[i], remain, &retLen, 1000);
            if (ret != LIBUSB_SUCCESS) {return "";}

            i += retLen;
            remain -= retLen;

            if (0 == remain) {
                break;
            }
        }

        if (j >= 5) {return "";} //错误

        inBuf.clear();
        inBuf.append(CH341_STREAM_TYPE_I2C);
    }

    //inBuf.append(CH341_I2C_STREAM_CMD_IN | 0);
    inBuf.append(CH341_I2C_STREAM_CMD_STOP);
    inBuf.append(1, CH341_I2C_STREAM_CMD_END);

    ret = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
              (uchar *)inBuf.data(), inBuf.length(), nullptr, 1000);

    if (ret != LIBUSB_SUCCESS) {return "";}

    return QByteArray((char *)outBuf, n);
}


int Ch341Interface::i2cWritePrivate(const QByteArray &in, uint addr)
{
    if (m_chipVersion < 0x20) {
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
    buf.append(CH341_STREAM_TYPE_I2C);
    buf.append(CH341_I2C_STREAM_CMD_START);
    len = CH341_PACKET_LENGTH - buf.length() - 2;
    if (len > in.length() + addrSize) {
        len = in.length() + addrSize;
    }
    buf.append(CH341_I2C_STREAM_CMD_OUT | len);

    buf.append(0xA0 | (addr >> ((addrSize - 1) * 8 - 1) & 0x0E));
    i = addrSize - 2;
    while (i > 0) {
        buf.append(addr >> (i * 8));
        ++i;
    }
    buf.append(addr);
    buf += in;

    i = CH341_PACKET_LENGTH - 1;
    len = CH341_PACKET_LENGTH - 3;

    while (i < buf.size() + 1) {
        buf.insert(i, (char) CH341_I2C_STREAM_CMD_END);
        buf.insert(i + 1, (char) CH341_STREAM_TYPE_I2C);
        if (buf.size() - i - 2 < len) {
            len = buf.size() - i - 2;
        }
        buf.insert(i + 2, CH341_I2C_STREAM_CMD_OUT | len);
        i += CH341_PACKET_LENGTH;
    }

    buf.append(CH341_I2C_STREAM_CMD_STOP);
    buf.append(1, CH341_I2C_STREAM_CMD_END);

    i = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
            (uchar *)buf.data(), buf.length(), &len, 1000);

    return (LIBUSB_SUCCESS == i);
}

