#ifndef SPECTRUM_ANALYZER_H
#define SPECTRUM_ANALYZER_H

#include <Arduino.h>

// -----------------------------------------------------------------------------
class MovingAverage
{
    private:
        enum{BUFFER_SIZE = 4};
        uint32_t m_buffer[BUFFER_SIZE];
        int m_rd_ptr;
        int m_wr_ptr;
        int m_count;
        uint32_t m_sum;
    public:
        MovingAverage() : m_rd_ptr(0), m_wr_ptr(0), m_count(0), m_sum(0){
        }
        void clear(){
            m_count = 0;
            m_rd_ptr = 0;
            m_wr_ptr = 0;
            m_sum = 0;
        }
        void append(uint32_t data){
            if( m_count < BUFFER_SIZE )
            {
                m_buffer[m_wr_ptr] = data;
                m_count++;
                m_sum += data;
            }
            else
            {
                m_sum -= m_buffer[m_rd_ptr];
                m_rd_ptr = (m_rd_ptr + 1) % BUFFER_SIZE;
                m_buffer[m_wr_ptr] = data;
                m_sum += data;
            }
            m_wr_ptr = (m_wr_ptr + 1) % BUFFER_SIZE;
        }
        uint32_t getValue(){
            if( m_count == 0 ){ return 0; }
            return m_sum / m_count;
        }
};

// -----------------------------------------------------------------------------
class SpectrumAnalyzer
{
    public:
        enum{NUM_CHANNELS = 7};
    private:
        enum{MINIMAL_GAIN = 50};
        uint8_t m_reset_pin;
        uint8_t m_strobe_pin;
        bool m_enabled;
        MovingAverage m_samples[NUM_CHANNELS];
    
    public:
        SpectrumAnalyzer();
        void begin(uint8_t res, uint8_t stb);
        void enable();
        void disable();
        void update();
        uint16_t getValue(int channel){ 
            return m_samples[channel].getValue(); 
        }
};

#endif
