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
    , m_vendorIc(0x00)
    , m_streamMode(0x81)
    , m_spiCS(0x80)
    , m_bulkInEndpointAddr(0)
    , m_bulkOutEndpointAddr(0)
{

}

bool Ch341Interface::openDevice()
{
    int err, i, count;
    m_error = OpenError;
    err = libusb_init(nullptr);
    if (err != LIBUSB_SUCCESS) {return false;}

    struct libusb_device **devices;
    struct libusb_device_descriptor devDescriptor;
    count = libusb_get_device_list(nullptr, &devices);
    m_devHandle = nullptr;

    for (i = 0; i < count; ++i) {
        if (libusb_get_port_number(devices[i]) != m_portNumber) {continue;}

        err = libusb_get_device_descriptor(devices[i], &devDescriptor);
        if (err != LIBUSB_SUCCESS) {continue;}

        if (CH341_VENDOR_ID != devDescriptor.idVendor ||
            CH341_PRODUCT_ID != devDescriptor.idProduct)
        {
            continue;
        }

        err = libusb_open(devices[i], &m_devHandle);
        if (err != LIBUSB_SUCCESS) {
            if (LIBUSB_ERROR_ACCESS == err) {m_error = PermissionError;}
            break;
        }

        if (libusb_kernel_driver_active(m_devHandle, 0) == 1) {
            if (libusb_detach_kernel_driver(m_devHandle, 0) != 0) {//卸载内核驱动
                break;
            }
        }

        int configs;
        err = libusb_get_configuration(m_devHandle, &configs);
        if (err != LIBUSB_SUCCESS) {break;}

        libusb_config_descriptor *configDesc;
        const libusb_endpoint_descriptor *endpDesc;
        err = libusb_get_config_descriptor(devices[i], 0, &configDesc);
        if (err != LIBUSB_SUCCESS) {break;}

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

        err = libusb_claim_interface(m_devHandle, 0);
        if (err != LIBUSB_SUCCESS) {break;}

        m_error = NoError;
        break;
    }

    if (i >= count) {m_error = DeviceNotFoundError;}

    libusb_free_device_list(devices, 1);
    if (m_error) {
        libusb_exit(nullptr);
    }else {
        this->vendorId(); //初始化 vendorIc 后才能正常使用
    }

    return !m_error;
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


QByteArray Ch341Interface::vendorId()
{
    int err;
    uchar data[2] = {0x00};
    m_vendorIc = 0x00;
    err = libusb_control_transfer(m_devHandle,
              LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
              CH341_REQ_CHIP_VERSION, 0x0000, 0x0000, data, 2, 1000);
    //此处 err 等于 2
    if (err < 0) {return "";}

    m_vendorIc = data[0];
    return QByteArray((char *)data, 2);
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


bool Ch341Interface::spiSendShortDataPrivate(const QByteArray &data)
{
    if (m_vendorIc < 0x20 || (m_vendorIc > 0x25 && m_vendorIc < 0x30)) {
        m_error = UnsupportedOperationError;
        return false;
    }

    QByteArray buf;
    if (m_spiCS & 0x80) {
        buf.append(CH341_CMD_UIO_STREAM);
        switch (m_spiCS & 0x03) {
        case 0x00: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x36); break; //DCK/D3->0, D0 ->0
        case 0x01: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x35); break; //DCK/D3->0, D1 ->0
        case 0x02: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x33); break; //DCK/D3->0, D2 ->0
        default: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x27); break; //DCK/D3->0, D4 ->0
        }

        buf.append(CH341_CMD_UIO_STREAM_DIRECT | 0x3F);
        buf.append(CH341_CMD_UIO_STREAM_END);
        buf.append(CH341_PACKET_LENGTH - buf.length() % CH341_PACKET_LENGTH, 0x00);
    }

    buf.append(CH341_CMD_SPI_STREAM);
    int i, len;

    for (i = 0; i < data.length(); ++i) {
        buf.append(msbLsbSwappedTable[(uchar)data.at(i)]);
    }

    i = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
            (uchar *)buf.data(), buf.length(), &len, 1000);

    return (LIBUSB_SUCCESS == i);
}

QByteArray Ch341Interface::spiReceiveShortDataPrivate(int len)
{
    uchar outBuf[len];
    int i, ret;
    i = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
              outBuf, len, &ret, 1000);
    if (i != LIBUSB_SUCCESS) {return "";}

    if (m_spiCS & 0x80) {
        QByteArray buf;
        buf.append(CH341_CMD_UIO_STREAM);
        buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x37);
        buf.append(CH341_CMD_UIO_STREAM_END);

        i = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  (uchar *)buf.data(), buf.length(), &ret, 1000);
        if (i != LIBUSB_SUCCESS) {return "";}
    }

    if (m_streamMode & 0x80) {
        i = 0;
        while (i < len) {
            outBuf[i] = msbLsbSwappedTable[outBuf[i]];
            ++i;
        }
    }

    return QByteArray((char*)outBuf, len);
}

QByteArray Ch341Interface::spiJedecId()
{
    QByteArray buf;
    buf.append(SPI_CMD_JEDEC_ID);
    buf.append(1, 0x00); //addr23-16
    buf.append(1, 0x00); //addr15-8
    buf.append(1, 0x00); //addr7-0

    if (!this->spiSendShortDataPrivate(buf)) {
        m_error = ReadError;
        return "";
    }

    buf = this->spiReceiveShortDataPrivate(4);
    if (buf.length()) {
        return buf.mid(1);
    }

    return "";
}


QByteArray Ch341Interface::spiReadPrivate(uint n, uint addr)
{
    if (m_vendorIc < 0x20 || (m_vendorIc > 0x25 && m_vendorIc < 0x30)) {
        m_error = UnsupportedOperationError;
        return "";
    }

    QByteArray buf;
    uchar outBuf[CH341_MAX_BUF_LEN + 128];

    if (m_spiCS & 0x80) {
        buf.append(CH341_CMD_UIO_STREAM);
        switch (m_spiCS & 0x03) {
        case 0x00: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x36); break; //DCK/D3->0, D0 ->0
        case 0x01: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x35); break; //DCK/D3->0, D1 ->0
        case 0x02: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x33); break; //DCK/D3->0, D2 ->0
        default: buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x27); break; //DCK/D3->0, D4 ->0
        }

        buf.append(CH341_CMD_UIO_STREAM_DIRECT | 0x3F);
        buf.append(CH341_CMD_UIO_STREAM_END);
        buf.append(CH341_PACKET_LENGTH - buf.length() % CH341_PACKET_LENGTH, 0x00);
    }

    buf.append(CH341_CMD_SPI_STREAM);

    buf.append(msbLsbSwappedTable[SPI_CMD_READ_DATA]); //cmd
    buf.append(msbLsbSwappedTable[(addr >> 16) & 0xFF]); //addr23 - 16
    buf.append(msbLsbSwappedTable[(addr >> 8) & 0xFF]); //addr15 - 8
    buf.append(msbLsbSwappedTable[addr & 0xFF]); //addr7 - 0
    buf.append(CH341_PACKET_LENGTH - buf.length() % CH341_PACKET_LENGTH, 0x00);

    int i = 0, err, len;
    err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
              (uchar *)buf.data(), buf.length(), &len, 1000);
    if (err != LIBUSB_SUCCESS) {return "";}

    err = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
              outBuf + i, CH341_EPP_IO_MAX, &len, 1000);
    if (err != LIBUSB_SUCCESS) {return "";}

    i += len;
    buf.clear();
    buf.append(CH341_CMD_SPI_STREAM);
    buf.append(CH341_PACKET_LENGTH - 1, 0x00);
    while (i < (int)n) {
        err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  (uchar *)buf.data(), buf.length(), &len, 1000);
        if (err != LIBUSB_SUCCESS) {return "";}

        err = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
                  outBuf + i, CH341_EPP_IO_MAX, &len, 1000);
        if (err != LIBUSB_SUCCESS) {return "";}

        i += len;
    }

    if (m_spiCS & 0x80) {
        buf.clear();
        buf.append(CH341_CMD_UIO_STREAM);
        buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x37);
        buf.append(CH341_CMD_UIO_STREAM_END);

        err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  (uchar *)buf.data(), buf.length(), &len, 1000);
        if (err != LIBUSB_SUCCESS) {return "";}
    }

    if (m_streamMode & 0x80) {
        i = n + 3;
        while (i >= 4) {
            outBuf[i] = msbLsbSwappedTable[outBuf[i]];
            --i;
        }
    }

    return QByteArray((char *)outBuf + 4, n);
}


inline QByteArray Ch341Interface::spiGetStatusPrivate(uchar cmd)
{

    QByteArray buf;

    buf.append(cmd);
    buf.append(1, 0x00);
    if (!this->spiSendShortDataPrivate(buf)) {return "";}

    buf = this->spiReceiveShortDataPrivate(2);
    if (buf.length()) {
        return buf.mid(1);
    }

    return "";
}

bool Ch341Interface::spiWriteEnablePrivate()
{
    if (!spiWaitPrivate()) {return false;}

    QByteArray buf;
    buf.append(SPI_CMD_WRITE_ENABLE);

    if (!this->spiSendShortDataPrivate(buf)) {return false;}

    if (m_spiCS & 0x80) {
        buf.clear();
        buf.append(CH341_CMD_UIO_STREAM);
        buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x37);
        buf.append(CH341_CMD_UIO_STREAM_END);

        int len, err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                          (uchar *)buf.data(), buf.length(), &len, 1000);
        if (err != LIBUSB_SUCCESS) {return false;}
    }

    return true;
}

bool Ch341Interface::spiWaitPrivate()
{
    QByteArray buf;

    while (1) {
        buf = spiGetStatusPrivate(SPI_CMD_READ_SR);

        if (!buf.length()) {return false;}

        if (!(buf.at(0) & SPI_STATUS_BUSY)) {return true;}
    }

    return false;
}

bool Ch341Interface::spiEraseChip()
{
   if (!spiWriteEnablePrivate()) {return false;}
   QByteArray buf;
   buf.append(SPI_CMD_ERASE_CHIP);

   if (!this->spiSendShortDataPrivate(buf)) {
       m_error = WriteError;
       return false;
   }

   if (m_spiCS & 0x80) {
       buf.clear();
       buf.append(CH341_CMD_UIO_STREAM);
       buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x37);
       buf.append(CH341_CMD_UIO_STREAM_END);

       int len, err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                          (uchar *)buf.data(), buf.length(), &len, 1000);
       if (err != LIBUSB_SUCCESS) {return false;}
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

    if (!this->spiSendShortDataPrivate(buf)) {
        m_error = WriteError;
        return false;
    }

    if (m_spiCS & 0x80) {
        buf.clear();
        buf.append(CH341_CMD_UIO_STREAM);
        buf.append((char)CH341_CMD_UIO_STREAM_OUT | 0x37);
        buf.append(CH341_CMD_UIO_STREAM_END);

        int len, err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                           (uchar *)buf.data(), buf.length(), &len, 1000);
        if (err != LIBUSB_SUCCESS) {return false;}
    }

    return spiWaitPrivate();
}

int Ch341Interface::spiWritePrivate(const QByteArray &in, uint addr)
{
    const uchar* origDataPtr = (const uchar*)in.data();
    uchar buf[(SPI_MAX_PAGE_PRAGRAM_SIZE
              + SPI_MAX_PAGE_PRAGRAM_SIZE / CH341_PACKET_LENGTH
              + CH341_PACKET_LENGTH + 2) << 1],
          readBuf[CH341_PACKET_LENGTH],
          endCmdBuf[3] = {
              CH341_CMD_UIO_STREAM,
              CH341_CMD_UIO_STREAM_OUT | 0x37,
              CH341_CMD_UIO_STREAM_END
          };

    int ret, err, headerLen, len, origDataLen, i, sum, pageSize, step;
    headerLen = len = sum = 0;
    if (m_spiCS & 0x80) {
        buf[0] = CH341_CMD_UIO_STREAM;
        switch (m_spiCS & 0x03) {
        case 0x00: buf[1] = CH341_CMD_UIO_STREAM_OUT | 0x36; break; //DCK/D3->0, D0 ->0
        case 0x01: buf[1] = CH341_CMD_UIO_STREAM_OUT | 0x35; break; //DCK/D3->0, D1 ->0
        case 0x02: buf[1] = CH341_CMD_UIO_STREAM_OUT | 0x33; break; //DCK/D3->0, D2 ->0
        default: buf[1] = CH341_CMD_UIO_STREAM_OUT | 0x27; break; //DCK/D3->0, D4 ->0
        }

        buf[2] = CH341_CMD_UIO_STREAM_DIRECT | 0x3F;
        buf[3] = CH341_CMD_UIO_STREAM_END;
        len = headerLen = CH341_PACKET_LENGTH;
    }

    pageSize = SPI_MAX_PAGE_PRAGRAM_SIZE - addr % SPI_MAX_PAGE_PRAGRAM_SIZE; //首页大小
    origDataLen = in.length();

    buf[len++] = CH341_CMD_SPI_STREAM;
    buf[len++] = msbLsbSwappedTable[SPI_CMD_PAGE_PROGRAM];
    buf[len++] = msbLsbSwappedTable[(addr >> 16) & 0xFF];
    buf[len++] = msbLsbSwappedTable[(addr >> 8) & 0xFF];
    buf[len++] = msbLsbSwappedTable[addr & 0xFF];

    if (origDataLen < pageSize) {
        pageSize = origDataLen;
    }

    step = pageSize;
    while (step-- > 0) {
        if (!(len % CH341_PACKET_LENGTH)) {
            if (0x20 == m_vendorIc) {
                buf[len] = buf[len + 1] = 0x00;
                len += CH341_PACKET_LENGTH;
            }
            buf[len++] = CH341_CMD_SPI_STREAM;
        }
        buf[len++] = msbLsbSwappedTable[*origDataPtr++];
    }

    if (!spiWriteEnablePrivate()) {
        m_error = WriteError;
        return -WriteError;
    }

    i = 0;
    step = CH341_PACKET_LENGTH;
    while (i < len) {
        if (step > len - i) {
            step = len - i;
        }
        err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                buf + i, step, &ret, 1000);
        if (err != LIBUSB_SUCCESS) {
            m_error = WriteError;
            return -WriteError;
        }
        i += step;

        libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
            readBuf, CH341_EPP_IO_MAX, &ret, 1000);
    }

    if (m_spiCS & 0x80) {
        err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                endCmdBuf, 3, &ret, 1000);
        if (err != LIBUSB_SUCCESS) {
            m_error = WriteError;
            return -WriteError;
        }
    }

    sum += pageSize;
    origDataLen -= pageSize;
    addr += pageSize;
    pageSize = SPI_MAX_PAGE_PRAGRAM_SIZE;

    while (origDataLen) {
        if (origDataLen < pageSize) {
            pageSize = origDataLen;
        }

        len = headerLen + 2;
        buf[len++] = msbLsbSwappedTable[(addr >> 16) & 0xFF];
        buf[len++] = msbLsbSwappedTable[(addr >> 8) & 0xFF];
        buf[len++] = msbLsbSwappedTable[addr & 0xFF];

        step = pageSize;
        while (step-- > 0) {
            if (!(len % CH341_PACKET_LENGTH)) {
                if (0x20 == m_vendorIc) {
                    buf[len] = buf[len + 1] = 0x00;
                    len += CH341_PACKET_LENGTH;
                }
                buf[len++] = CH341_CMD_SPI_STREAM;
            }
            buf[len++] = msbLsbSwappedTable[*origDataPtr++];
        }

        if (!spiWriteEnablePrivate()) {
            m_error = WriteError;
            return -WriteError;
        }

        i = 0;
        step = CH341_PACKET_LENGTH;
        while (i < len) {
            if (step > len - i) {
                step = len - i;
            }
            err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                    buf + i, step, &ret, 1000);
            if (err != LIBUSB_SUCCESS) {
                m_error = WriteError;
                return -WriteError;
            }

            i += step;

            libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
                readBuf, CH341_EPP_IO_MAX, &ret, 1000);
        }

        if (m_spiCS & 0x80) {
            err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                    endCmdBuf, 3, &ret, 1000);
            if (err != LIBUSB_SUCCESS) {
                m_error = WriteError;
                return -WriteError;
            }
        }

        sum += pageSize;
        addr += pageSize;
        origDataLen -= pageSize;
    }

    spiWaitPrivate();
    return sum;
}


QByteArray Ch341Interface::i2cReadPrivate(uint n, uint addr, uchar addrSize)
{
    if (m_vendorIc < 0x20) {
        m_error = UnsupportedOperationError;
        return "";
    }

    uchar outBuf[CH341_MAX_BUF_LEN + 128];
    QByteArray inBuf;

    inBuf.append(CH341_CMD_I2C_STREAM);

    inBuf.append(CH341_CMD_I2C_STREAM_START);
    inBuf.append(CH341_CMD_I2C_STREAM_OUT | addrSize);

    inBuf.append(0xA0 | ((addr >> ((addrSize - 1) * 8 - 1)) & 0x0E));
    int i = addrSize - 2;
    while (i > 0) {
        inBuf.append(addr >> (i * 8));
        --i;
    }
    inBuf.append(addr);

    inBuf.append(CH341_CMD_I2C_STREAM_START);
    inBuf.append((char) CH341_CMD_I2C_STREAM_OUT | 1);
    inBuf.append(0xA0 | ((addr >> ((addrSize - 1) * 8 - 1)) & 0x0E) | 0x01); //器件地址（读）

    int step = CH341_EPP_IO_MAX;
    int err, len;
    i = n;
    while (i > 0) {
        if (i < step) {
            step = i;
        }

        inBuf.append(CH341_CMD_I2C_STREAM_IN | step);
        if (step >= CH341_EPP_IO_MAX) {
            inBuf.append(1, CH341_CMD_I2C_STREAM_END);
            inBuf.append(CH341_PACKET_LENGTH - inBuf.length() % CH341_PACKET_LENGTH, 0x00);
        }

        err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
                  (uchar *)inBuf.data(), inBuf.length(), &len, 1000);
        if (err != LIBUSB_SUCCESS) {return "";}

        err = libusb_bulk_transfer(m_devHandle, m_bulkInEndpointAddr,
                  outBuf + n - i, step, &len, 1000);
        if (err != LIBUSB_SUCCESS) {return "";}

        i -= len;
        inBuf.clear();
        inBuf.append(CH341_CMD_I2C_STREAM);
    }

    inBuf.append(CH341_CMD_I2C_STREAM_IN);
    inBuf.append(CH341_CMD_I2C_STREAM_STOP);
    inBuf.append(1, CH341_CMD_I2C_STREAM_END);

    err = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
              (uchar *)inBuf.data(), inBuf.length(), &len, 1000);
    if (err != LIBUSB_SUCCESS) {return "";}

    return QByteArray((char *)outBuf, n);
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
    buf.append(CH341_CMD_I2C_STREAM);
    buf.append(CH341_CMD_I2C_STREAM_START);
    len = CH341_PACKET_LENGTH - buf.length() - 2;
    if (len > in.length() + addrSize) {
        len = in.length() + addrSize;
    }
    buf.append(CH341_CMD_I2C_STREAM_OUT | len);

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
        buf.insert(i, (char) CH341_CMD_I2C_STREAM_END);
        buf.insert(i + 1, (char) CH341_CMD_I2C_STREAM);
        if (buf.size() - i - 2 < len) {
            len = buf.size() - i - 2;
        }
        buf.insert(i + 2, CH341_CMD_I2C_STREAM_OUT | len);
        i += CH341_PACKET_LENGTH;
    }

    buf.append(CH341_CMD_I2C_STREAM_STOP);
    buf.append(1, CH341_CMD_I2C_STREAM_END);

    i = libusb_bulk_transfer(m_devHandle, m_bulkOutEndpointAddr,
            (uchar *)buf.data(), buf.length(), &len, 1000);

    return (LIBUSB_SUCCESS == i);
}

