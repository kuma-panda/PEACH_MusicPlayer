#ifndef M41T62_H
#define M41T62_H

#include <Arduino.h>
#include <Wire.h>

class M41T62 
{
    private:
		enum RegisterAddr 
		{
			M41T62_100THS_SEC= 0,
			M41T62_SEC,
			M41T62_MIN,
			M41T62_HOUR,
			M41T62_DOW,
			M41T62_DATE,
			M41T62_CENT_MONTH,
			M41T62_YEAR,
			M41T62_CALIB,
			M41T62_WDMB,
			M41T62_ALARM_MONTH,
			M41T62_ALARM_DATE,
			M41T62_ALARM_HOUR,
			M41T62_ALARM_NIM,
			M41T62_ALARM_SEC,
			M41T62_WDFLAG,
		};
		enum{MT41T62_SLAVEADDR = 0b1101000};

		uint16_t m_year;
		uint16_t m_month;
		uint16_t m_day;
		uint16_t m_hour;
		uint16_t m_minute;
		uint16_t m_second;

		static uint16_t BCD2DEC(uint8_t bcd);
		static uint8_t DEC2BCD(uint16_t dec);

        void readRegisters(uint8_t reg, uint8_t *, uint8_t num);
        void writeRegisters(uint8_t reg, uint8_t *, uint8_t num);
		void updateTime();
		void updateCalendar();
		uint16_t getDayOfWeek(uint16_t year, uint16_t month, uint16_t day); 

		M41T62();

	public:
		static M41T62& getInstance();
		void update(){
			updateTime();
			updateCalendar();
		}
		void set(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second=0);
		void get(uint16_t& year, uint16_t& month, uint16_t& day, uint16_t& wday, uint16_t& hour, uint16_t& minute, uint16_t& second);
		void getAsText(char *buffer, bool digit_only);
};

#endif
