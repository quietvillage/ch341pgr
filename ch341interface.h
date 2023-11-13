/*
 * Ch341Interface类提供了 CH341 系列芯片的 SPI、I2C 接口，
 * 是依据沁恒官方（ 网址：www.wch.cn ） CH341A 接口库重写的
*/
#ifndef CH341_INTERFACE_H
#define CH341_INTERFACE_H

#include "libusb/libusb.h"
#include <QString>

#define CH341_VENDOR_ID     0x1A86
#define CH341_PRODUCT_ID    0x5512

#define CH341_REQ_CHIP_VERSION  0x5F //请求版本

#define CH341_MAX_BUF_LEN     4096
#define CH341_DEFAULT_BUF_LEN 1024
#define CH341_PACKET_LENGTH   32
#define CH341_EPP_IO_MAX  (CH341_PACKET_LENGTH - 1)

#define CH341_CMD_SPI_STREAM   0xA8    //SPI Interface Command
#define CH341_CMD_I2C_STREAM   0xAA    // I2C Interface Command
#define CH341_CMD_UIO_STREAM   0xAB    // UIO Interface Command
#define CH341_CMD_PIO_STREAM   0xAE    // PIO Interface Command

#define OPERATION_SPI_STREAM_READ   true
#define OPERATION_SPI_STREAM_WRITE  false

#define CH341_CMD_I2C_STREAM_START     0x74    //I2C Stream Start Command
#define CH341_CMD_I2C_STREAM_STOP      0x75    //I2C Stream Stop byte Command
#define CH341_CMD_I2C_STREAM_OUT       0x80    //I2C Stream Out Command
#define CH341_CMD_I2C_STREAM_IN        0xC0    //I2C Stream In Command
#define CH341_CMD_I2C_STREAM_MAX qMin(0x3F, CH341_PACKET_LENGTH)   //I2C Stream Max Length
#define CH341_CMD_I2C_STREAM_SET       0x60    //I2C Stream Set Mode

//-->bit2   spi io (0: one in one out ; 1: two in two out)
//-->bit1~0  I2C SCL Rate
#define CH341_CMD_I2C_STREAM_DELAY_US  0x40    //I2C Stream Delay(us)
#define CH341_CMD_I2C_STREAM_DELAY_MS  0x50    //I2C Stream Delay(ms)
#define CH341_CMD_I2C_STREAM_DELAY     0x0F    //I2C Stream Set Max Delay
#define CH341_CMD_I2C_STREAM_END       0x00    //I2C Stream End Command

#define CH341_CMD_UIO_STREAM_IN        0x00    // UIO Interface In ( D0 ~ D7 )
#define CH341_CMD_UIO_STREAM_DIRECT    0x40    // UIO interface Dir( set dir of D0~D5 )
#define CH341_CMD_UIO_STREAM_OUT       0x80    // UIO Interface Output(D0~D5)
#define CH341_CMD_UIO_STREAM_DELAY_US  0xC0    // UIO Interface Delay Command( us )
#define CH341_CMD_UIO_STREAM_END       0x20    // UIO Interface End Command

class Ch341Interface
{
public:
    Ch341Interface();

    enum Ch341InterfaceError {
        NoError = 0,
        DeviceNotFoundError,
        PermissionError,
        OpenError,
        ReadError,
        WriteError,
        UnsupportedOperationError
    };

    enum MemType {
        SpiFlash = 0,
        I2cEEPROM
    };

    bool openDevice();
    void closeDevice();

    QString errorMessage(int code = -1);
    void setPortNumber(int port) {m_portNumber = port;}
    void setChipModel(enum MemType type, int modelIndex)
    {
        m_memType =  type;
        m_modelIndex = modelIndex;
    }

    QByteArray vendorId();

    QByteArray read(uint n, uint addr = 0);
    int write(const QByteArray &data, uint addr = 0);

    void spiSetCS(uchar cs) {m_spiCS = 0x80 | (cs & 0x03);}
    QByteArray spiJedecId();
    bool spiEraseChip();
    bool spiEraseSector(uint addr);

private:
    bool spiSendShortDataPrivate(const QByteArray &data);
    QByteArray spiReceiveShortDataPrivate(int len);
    inline QByteArray spiGetStatusPrivate(uchar cmd);
    bool spiWriteEnablePrivate();
    bool spiWaitPrivate();

    QByteArray spiReadPrivate(uint n, uint addr);
    int spiWritePrivate(const QByteArray &in, uint addr);

    //addrSize - 地址字节数 对于 24C16 及以下是 2 字节，往上的型号则 3 个字节
    QByteArray i2cReadPrivate(uint n, uint addr, uchar addrSize);

    int i2cWritePrivate(const QByteArray &in, uint addr);
    bool i2cPageProgramPrivate(const QByteArray &in, uint addr, uchar addrSize);


    libusb_device_handle *m_devHandle;
    int m_portNumber; //当前端口号
    enum Ch341InterfaceError m_error;
    enum MemType m_memType;
    int m_modelIndex;
    uchar m_vendorIc;
    uchar m_streamMode; //最高位为数据发送模式 1-低位先发送 0-高位先发送
    uchar m_spiCS; //chip select
    uint8_t m_bulkInEndpointAddr;
    uint8_t m_bulkOutEndpointAddr;
};

#endif // CH341_INTERFACE_H
