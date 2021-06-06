#include <Arduino.h>
#include "ir_remote.h"

// -----------------------------------------------------------------------------
IRRemote::IRRemote() : m_value(0)
{

}

// -----------------------------------------------------------------------------
void IRRemote::begin()
{
    Serial5.begin(9600);
}

// -----------------------------------------------------------------------------
uint32_t IRRemote::read()
{
    if( Serial5.available() )
    {
        return decode(Serial5.read());
    }
    return 0;
}

// -----------------------------------------------------------------------------
uint32_t IRRemote::decode(char c)
{
    // Serial.print(c);
    if( '0' <= c && c <= '9' )
    {
        uint32_t d = (uint32_t)(c - '0');
        m_value <<= 4;
        m_value += d;
    }
    else if( 'a' <= c && c <= 'f' )
    {
        uint32_t d = (uint32_t)((c - 'a') + 10);
        m_value <<= 4;
        m_value += d;
    }
    else if( 'A' <= c && c <= 'F' )
    {
        uint32_t d = (uint32_t)((c - 'A') + 10);
        m_value <<= 4;
        m_value += d;
    }
    else
    {
        uint32_t d = m_value;
        m_value = 0;
        if( d < 0xF0000000 )
        {
            return d;
        }
    }
    return 0;
}

// -----------------------------------------------------------------------------
int IRRemote::codeToDigit(IRRCODE code)
{
    IRRCODE digit_map[] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE};
    for( int n = 0 ; n < 10 ; n++ )
    {
        if( code == digit_map[n] )
        {
            return n;
        }
    }
    return -1;
}