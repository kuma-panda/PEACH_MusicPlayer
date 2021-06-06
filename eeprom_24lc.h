#ifndef EEPROM_24LC_H
#define EEPROM_24LC_H

#include <Arduino.h>

class EEPROM_24LC
{
    friend EEPROM_24LC& EEPROM();
    private:
        enum{I2C_ADDR = 0x50};
        EEPROM_24LC(){}
    public:
        void read(uint16_t addr, uint16_t size, uint8_t *buffer);
        void write(uint16_t addr, uint16_t size, uint8_t *data);
};

EEPROM_24LC& EEPROM();

#endif
