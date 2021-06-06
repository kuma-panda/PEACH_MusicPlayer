#include "spectrum_analyzer.h"

#define MSGEQ7_DATA_OUT A0

// -----------------------------------------------------------------------------
SpectrumAnalyzer::SpectrumAnalyzer() : m_enabled(false)
{
}

// -----------------------------------------------------------------------------
void SpectrumAnalyzer::begin(uint8_t res, uint8_t stb)
{
    m_reset_pin = res;
    m_strobe_pin = stb;
    pinMode(m_reset_pin, OUTPUT);
    pinMode(m_strobe_pin, OUTPUT);
    pinMode(MSGEQ7_DATA_OUT, INPUT);
}

// -----------------------------------------------------------------------------
void SpectrumAnalyzer::enable()
{
    if( !m_enabled )
    {
        for( int ch = 0 ; ch < NUM_CHANNELS ; ch++ )
        {
            m_samples[ch].clear();
        }
        m_enabled = true;
    }
}

// -----------------------------------------------------------------------------
void SpectrumAnalyzer::disable()
{
    if( m_enabled )
    {
        m_enabled = false;
        for( int ch = 0 ; ch < NUM_CHANNELS ; ch++ )
        {
            m_samples[ch].clear();
        }
    }
}

// -----------------------------------------------------------------------------
void SpectrumAnalyzer::update()
{
    if( !m_enabled )
    {
        return;
    }
    uint32_t data[NUM_CHANNELS];
    uint32_t average = 0;
    digitalWrite(m_strobe_pin, LOW);
    digitalWrite(m_reset_pin, HIGH);
    delayMicroseconds(1);
    digitalWrite(m_reset_pin, LOW);
    delayMicroseconds(75);
    for( int ch = 0 ; ch < NUM_CHANNELS ; ch++ )
    {
        digitalWrite(m_strobe_pin, LOW);
        delayMicroseconds(40);
        data[ch] = analogRead(MSGEQ7_DATA_OUT);
        if( ch == 0 || ch == NUM_CHANNELS-1 )
        {
            data[ch] = data[ch] * 3 / 5;
        }
        else
        {
            average += data[ch];
        }
        digitalWrite(m_strobe_pin, HIGH);
        delayMicroseconds(40);
    }
    average /= (NUM_CHANNELS-2);
    for( int ch = 0 ; ch < NUM_CHANNELS ; ch++ )
    {
        if( average < MINIMAL_GAIN )
        {
            // 無音状態、とみなす
            m_samples[ch].append(0);    
        }
        else
        {
            if( data[ch] < MINIMAL_GAIN )
            {
                m_samples[ch].append(0);
            }
            else
            {
                m_samples[ch].append(data[ch] - MINIMAL_GAIN);
            }
        }
    }
}
