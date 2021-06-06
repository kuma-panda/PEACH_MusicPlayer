#include "M41T62.h"
#include <Wire.h>

// -----------------------------------------------------------------------------
M41T62& M41T62::getInstance()
{
	static M41T62 _rtc;
	return _rtc;
}

// -----------------------------------------------------------------------------
M41T62::M41T62() 
	: m_year(2020), m_month(12), m_day(1), m_hour(0), m_minute(0), m_second(0)
{

}

// -----------------------------------------------------------------------------
uint16_t M41T62::BCD2DEC(uint8_t bcd)
{
	return 10*(bcd >> 4) + (bcd & 0x0F);
}

// -----------------------------------------------------------------------------
uint8_t M41T62::DEC2BCD(uint16_t dec)
{
	dec %= 100;
	return (uint8_t)((dec / 10) << 4) + (uint8_t)(dec % 10);
}

// -----------------------------------------------------------------------------
void M41T62::readRegisters(uint8_t addr, uint8_t * regvals, uint8_t num) 
{
	Wire.beginTransmission(MT41T62_SLAVEADDR);
 	Wire.write(addr);
	Wire.endTransmission();
	Wire.requestFrom((uint8_t)MT41T62_SLAVEADDR, num);
	for(uint8_t i = 0 ; i < num ; i++ ) 
    {
		*regvals++ = Wire.read();
	}
}

// -----------------------------------------------------------------------------
void M41T62::writeRegisters(uint8_t addr, uint8_t *regvals, uint8_t num)
{
	Wire.beginTransmission(MT41T62_SLAVEADDR);
	Wire.write(addr); // reset register pointer
	for( uint8_t i = 0 ; i < num ; i++ ) 
    {
		Wire.write(*regvals++);
	}
	Wire.endTransmission();
}

// -----------------------------------------------------------------------------
void M41T62::updateTime() 
{
	uint8_t buffer[3];
	readRegisters((uint8_t)M41T62_SEC, buffer, 3);
	m_second = BCD2DEC(buffer[0] & 0x7F);
	m_minute = BCD2DEC(buffer[1] & 0x7F);
	m_hour   = BCD2DEC(buffer[2] & 0x3F);
}

// -----------------------------------------------------------------------------
void M41T62::updateCalendar() 
{
	uint8_t buffer[3];
	readRegisters((uint8_t)M41T62_DATE, buffer, 3);
	m_day   = BCD2DEC(buffer[0] & 0x3F);
	m_month = BCD2DEC(buffer[1] & 0x1F);
	m_year  = BCD2DEC(buffer[2]) + 2000;
}

// -----------------------------------------------------------------------------
void M41T62::set(uint16_t year, uint16_t month, uint16_t day,
							uint16_t hour, uint16_t minute, uint16_t second)
{
	uint8_t buffer[3];
	readRegisters((uint8_t)M41T62_SEC, buffer, 3);
	buffer[0] = (buffer[0] & 0x80) | (DEC2BCD(second) & 0x7F);
	buffer[1] = (buffer[1] & 0x80) | (DEC2BCD(minute) & 0x7F);
	buffer[2] = (buffer[2] & 0xC0) | (DEC2BCD(hour) & 0x3F);
	writeRegisters((uint8_t)M41T62_SEC, buffer, 3);

	readRegisters((uint8_t)M41T62_DATE, buffer, 3);
	buffer[0] = (buffer[0] & 0xC0) | (DEC2BCD(day) & 0x3F);
	buffer[1] = (buffer[1] & 0xE0) | (DEC2BCD(month) & 0x1F);
	buffer[2] = DEC2BCD(year-2000);
	writeRegisters((uint8_t)M41T62_DATE, buffer, 3);
}

// -----------------------------------------------------------------------------
void M41T62::get(uint16_t& year, uint16_t& month, uint16_t& day,
		uint16_t& week, uint16_t& hour, uint16_t& minute, uint16_t& second)
{
	year = m_year;
	month = m_month;
	day = m_day;
	week = getDayOfWeek(m_year, m_month, m_day);
	hour = m_hour;
	minute = m_minute;
	second = m_second;
}

// -----------------------------------------------------------------------------
uint16_t M41T62::getDayOfWeek(uint16_t year, uint16_t month, uint16_t day) 
{
	// 「ツェラーの公式」
	// 「１月」「２月」はそれぞれ「前年の13月、前年の14月」とみなす
	if( month < 3 )
	{
		year--;
		month += 12;
	}
	return (year + year/4 - year/100 + year/400 + (13 * month + 8)/5 + day) % 7;
	// 日曜日が0、土曜日が6
}

// -----------------------------------------------------------------------------
void M41T62::getAsText(char *buffer, bool digit_only)
{
	char *p = buffer;
	*p++ = '0' + (m_year / 1000);
	*p++ = '0' + ((m_year % 1000) / 100);
	*p++ = '0' + ((m_year % 100) / 10);
	*p++ = '0' + (m_year % 10);
	if( !digit_only )
	{
		*p++ = '-';
	}
	*p++ = '0' + (m_month / 10);
	*p++ = '0' + (m_month % 10);
	if( !digit_only )
	{
		*p++ = '-';
	}
	*p++ = '0' + (m_day / 10);
	*p++ = '0' + (m_day % 10);
	if( !digit_only )
	{
		*p++ = ' ';
	}
	*p++ = '0' + (m_hour / 10);
	*p++ = '0' + (m_hour % 10);
	if( !digit_only )
	{
		*p++ = ':';
	}
	*p++ = '0' + (m_minute / 10);
	*p++ = '0' + (m_minute % 10);
	*p = 0;
}

