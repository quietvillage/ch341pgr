#ifndef CHIP_DATA_H
#define CHIP_DATA_H

#include <QStringList>

// manufacturer ID
#define SPI_VENDOR_ID_WINBOND       0xef
#define SPI_VENDOR_ID_MXIC          0xc2
#define SPI_VENDOR_ID_GIGADEVICE    0xC8

#define SPI_MAX_PAGE_PRAGRAM_SIZE 256

// status register-1 mask
#define SPI_STATUS_BUSY     0x01
#define SPI_STATUS_WEL      0x02    // write enable

#define SPI_CMD_WRITE_ENABLE    0x06
#define SPI_CMD_READ_SR         0x05    // read status register

#define SPI_CMD_WRITE_SR        0x01    // write status register
#define SPI_CMD_PAGE_PROGRAM    0x02
#define SPI_CMD_ERASE_4KB       0x20    // sector erase
#define SPI_CMD_ERASE_32KB      0x52    // 32kb block erase
#define SPI_CMD_ERASE_64KB      0xd8    // 64kb block erase
#define SPI_CMD_ERASE_CHIP      0x60    // chip erase

#define SPI_CMD_READ_DATA       0x03
#define SPI_CMD_WENDOR_DEVICE   0x90
#define SPI_CMD_JEDEC_ID        0x9f    // JEDEC standard compatible ID
#define SPI_CMD_RESET_ENABLE    0x66
#define SPI_CMD_RESET           0x99
#define SPI_CMD_ENTER_4BYTE_ADDR_MODE   0xB7
#define SPI_CMD_EXIT_4BYTE_ADDR_MODE    0xE9

#define SPI_FLASH_MODEL_INDEX_25Q256    10

#define EEPROM_MODEL_INDEX_24C01    1
#define EEPROM_MODEL_INDEX_24C02    2
#define EEPROM_MODEL_INDEX_24C04    3
#define EEPROM_MODEL_INDEX_24C16    5
#define EEPROM_MODEL_INDEX_24C64    7
#define EEPROM_MODEL_INDEX_24C256   9
#define EEPROM_MODEL_INDEX_24C512   10
#define EEPROM_MODEL_INDEX_24C4096  13

extern const QList<QStringList> chipModelLists;

#endif // CHIP_DATA_H
