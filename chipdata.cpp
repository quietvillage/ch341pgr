#include "chipdata.h"
#include <QObject>

const QList<QStringList> chipModelLists = {
    //25 SPI Flash
    {
        QObject::tr("未知"),
        "25x10",
        "25x20",
        "25x40",
        "25x80",
        "25x16",
        "25x32",
        "25x64",
        "25x128",
        "25x256",
        "25x512",
        "25x01"
    },

    //24 EEPROM
    {
        QObject::tr("未知"),
        "24C01",
        "24C02",
        "24C04",
        "24C08",
        "24C16",
        "24C32",
        "24C64",
        "24C128",
        "24C512",
        "24C1024",
        "24C2048",
        "24C4096"
    }
};
