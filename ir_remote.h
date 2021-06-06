#ifndef IR_REMOTE_H
#define IR_REMOTE_H

#include <Arduino.h>

typedef uint32_t    IRRCODE;

class IRRemote
{
    public:
        enum
        {
            POWER      = 0xFD00FF,
            ONE        = 0xFD08F7,
            DOWN       = 0xFD10EF,
            SEVEN      = 0xFD18E7,
            PREV       = 0xFD20DF,
            FOUR       = 0xFD28D7,
            ZERO       = 0xFD30CF,
            FN_STOP    = 0xFD40BF,
            THREE      = 0xFD48B7,
            UP         = 0xFD50AF,
            NINE       = 0xFD58A7,
            NEXT       = 0xFD609F,
            SIX        = 0xFD6897,
            ST_REPT    = 0xFD708F,
            VOL_UP     = 0xFD807F,
            TWO        = 0xFD8877,
            VOL_DOWN   = 0xFD906F,
            EIGHT      = 0xFD9867,
            PLAY_PAUSE = 0xFDA05F,
            FIVE       = 0xFDA857,
            EQ         = 0xFDB04F
        };
    private:
        IRRCODE m_value;
        IRRCODE decode(char c);
    public:
        IRRemote();
        void begin();
        IRRCODE read();
        static int codeToDigit(IRRCODE code);
};

#endif
