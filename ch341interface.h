/*
 * Ch341Interface类提供了 CH341 系列芯片的 SPI、I2C 接口，
 * 是依据沁恒官方（ 网址：www.wch.cn ） CH341A 接口库重写的
*/
#ifndef CH341_INTERFACE_H
#define CH341_INTERFACE_H

#include "libusb/libusb.h"
#include <QString>
#include <QPair>

#define CH341_VENDOR_ID     0x1A86
#define CH341_PRODUCT_ID    0x5512

#define CH341_REQ_CHIP_VERSION  0x5F //请求版本

#define CH341_MAX_BUF_LEN     4096
#define CH341_DEFAULT_BUF_LEN 1024
#define CH341_PACKET_LENGTH   32
#define CH341_EPP_IO_MAX  (CH341_PACKET_LENGTH - 1)

//放在包的第一个字节，标识数据流类型
#define CH341_STREAM_TYPE_SPI   0xA8    //  SPI
#define CH341_STREAM_TYPE_I2C   0xAA    // I2C
#define CH341_STREAM_TYPE_UIO   0xAB    // Universal IO
#define CH341_STREAM_TYPE_PARA  0xAE    // Parallel IO

#define CH341_I2C_STREAM_CMD_START     0x74    //发起 I2C 起始条件
#define CH341_I2C_STREAM_CMD_STOP      0x75    //产生 I2C 停止条件
#define CH341_I2C_STREAM_CMD_OUT       0x80    //I2C Stream Out Command
#define CH341_I2C_STREAM_CMD_IN        0xC0    //I2C Stream In Command
#define CH341_I2C_STREAM_CMD_SET_MODE  0x60    //设置 I2C 时钟频率（部分芯片可用）
#define CH341_I2C_STREAM_CMD_DELAY_US  0x40    //I2C Stream Delay(us)
#define CH341_I2C_STREAM_CMD_DELAY_MS  0x50    //I2C Stream Delay(ms)
#define CH341_I2C_STREAM_CMD_DELAY     0x0F    //I2C Stream Set Max Delay
#define CH341_I2C_STREAM_CMD_END       0x00    //I2C Stream End Command

#define CH341_UIO_STREAM_CMD_IN        0x00    // UIO Interface In ( D0 ~ D7 )
#define CH341_UIO_STREAM_CMD_DIRECT    0x40    // UIO interface Dir( set dir of D0~D5 )
#define CH341_UIO_STREAM_CMD_OUT       0x80    // UIO Interface Output(D0~D5)
#define CH341_UIO_STREAM_CMD_DELAY_US  0xC0    // UIO Interface Delay Command( us )
#define CH341_UIO_STREAM_CMD_END       0x20    // UIO Interface End Command

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
        UnsupportedOperationError,
        OverflowError
    };

    enum MemType {
        SpiFlash = 0,
        I2cEEPROM
    };

    bool openDevice();
    void closeDevice();

    QString errorMessage(int code = -1);
    void setPort(const QPair<int, int> &port) {m_port = port;}
    void setChipModel(enum MemType type, int modelIndex)
    {
        m_memType =  type;
        m_modelIndex = modelIndex;
    }

   bool getChipVersion();

    QByteArray read(uint n, uint addr = 0);
    int write(const QByteArray &data, uint addr = 0);

    void spiSetCS(uchar cs) {m_spiCS = cs & 0x03;}
    QByteArray spiJedecId();
    bool spiEraseChip();
    bool spiEraseSector(uint addr);

private:
    bool spiStreamStartPrivate();   //发起 spi 启动条件（拉低CS、CLK）
    bool spiStreamStopPrivate();    //发出 spi 结束条件（拉高CS）
    QByteArray spiCmdStreamPrivate(const QByteArray &in, int readLen);
    inline QByteArray spiGetStatusPrivate(uchar cmd);
    bool spiWriteEnablePrivate();
    bool spiWaitPrivate();

    QByteArray spiReadPrivate(uint n, uint addr);
    int spiWritePrivate(const QByteArray &in, uint addr);
    bool spiPageProgramPrivate(const QByteArray &in, uint addr);

    //addrSize - 地址字节数 对于 24C16 及以下是 2 字节，往上的型号则 3 个字节
    QByteArray i2cReadPrivate(uint n, uint addr, uchar addrSize);

    int i2cWritePrivate(const QByteArray &in, uint addr);
    bool i2cPageProgramPrivate(const QByteArray &in, uint addr, uchar addrSize);


    libusb_device_handle *m_devHandle;
    QPair<int, int> m_port; //当前端口号
    enum Ch341InterfaceError m_error;
    enum MemType m_memType;
    int m_modelIndex;
    uchar m_chipVersion; //0x20->CH341A, 0x30->CH341A3, 0x31、0x32->CH341B
    uchar m_spiCS; // CS line: 0x00->D0, 0x01->D1, 0x02->D2, 0x03->D4
    uint8_t m_bulkInEndpointAddr;
    uint8_t m_bulkOutEndpointAddr;
};

#endif // CH341_INTERFACE_H
