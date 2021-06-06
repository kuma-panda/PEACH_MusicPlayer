#include "eeprom_24lc.h"
#include <Wire.h>

// -----------------------------------------------------------------------------
EEPROM_24LC& EEPROM()
{
    static EEPROM_24LC _eeprom;
    return _eeprom;
}

// -----------------------------------------------------------------------------
void EEPROM_24LC::read(uint16_t addr, uint16_t size, uint8_t *buffer)
{
    for( uint16_t i = 0 ; i < size ; i++, addr++ )
    {
        Wire.beginTransmission(I2C_ADDR);
        // 対象アドレスに移動
        Wire.write((uint8_t)(addr >> 8));       // Address(High Byte)   
        Wire.write((uint8_t)(addr & 0x00FF));   // Address(Low Byte)
        Wire.endTransmission();

        // デバイスへ1byteのレジスタデータを要求する
        Wire.requestFrom(I2C_ADDR, 1);
        while (Wire.available() == 0 ){}
        buffer[i] = Wire.read();
    }
}

// -----------------------------------------------------------------------------
void EEPROM_24LC::write(uint16_t addr, uint16_t size, uint8_t *data)
{
    for( uint16_t i = 0 ; i < size ; i++, addr++ )
    {
        Wire.beginTransmission(I2C_ADDR);    
        // 対象アドレスに移動
        Wire.write((uint8_t)(addr >> 8));       // Address(High Byte)   
        Wire.write((uint8_t)(addr & 0x00FF));   // Address(Low Byte)
        // データの書き込み
        Wire.write(data[i]);
        Wire.endTransmission();
        delay(5);
    }
}
