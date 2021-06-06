#ifndef PLAYER_H
#define PLAYER_H

#include <SPI.h>
#include <SD.h>
#include "VS1053.h"

//------------------------------------------------------------------------------
class PlayerTimeCounter
{
    private:
        uint32_t m_startTime;
        uint32_t m_stopTime;
        bool     m_active;
    public:
        PlayerTimeCounter() : m_startTime(0), m_stopTime(0), m_active(false){}
        uint32_t getValue(){
            if( !m_active )
            {
                return (m_stopTime - m_startTime)/1000;
            }
            return (millis() - m_startTime)/1000;
        }
        void start(){
            m_startTime = millis() - (m_stopTime - m_startTime);
            m_active = true;
        }
        void stop(){
            m_stopTime = millis();
            m_active = false;
        }
        void reset(){
            m_startTime = 0;
            m_stopTime = 0;
            m_active = false;
        }
        bool isActive(){
            return m_active;
        }
};


class CommandFIFO
{
    private:
        enum{SIZE = 8};
        uint8_t m_buffer[SIZE];
        uint8_t m_rd;
        uint8_t m_wr;
        uint8_t m_len;
        bool    m_locked;
    public:
        enum{EMPTY = 0};
        CommandFIFO() : m_rd(0), m_wr(0), m_len(0), m_locked(false){}
        void push(uint8_t c){
            if( m_len >= SIZE )
            {
                return;
            }
            while( m_locked ){}
            m_locked = true;
            m_buffer[m_wr] = c;
            m_wr = (m_wr + 1) % SIZE;
            m_len++;
            m_locked = false;
        }
        uint8_t pop(){
            if( m_locked || !m_len )
            {
                return EMPTY;
            }
            m_locked = true;
            uint8_t c = m_buffer[m_rd];
            m_rd = (m_rd + 1) % SIZE;
            m_len--;
            m_locked = false;
            return c;
        }
};

// -----------------------------------------------------------------------------
class MusicPlayer
{
    friend MusicPlayer& Player();
    public:
        typedef void (*PLAYING_PROC)(void);
        enum{VOLUME_MAX = 20};
        enum{BASS_MAX = 10};
        enum{TREBLE_MAX = 7};
        enum{DEFAULT_VOLUME_VALUE = 4};

    private:
        // 本番基板
        enum{BREAKOUT_RESET = 26};  // VS1053 reset pin (output)
        enum{BREAKOUT_CS = 27};      // VS1053 chip select pin (output)
        enum{BREAKOUT_DCS = 29};     // VS1053 Data/command select pin (output)
        enum{CARDCS = 5};           // Card chip select pin
        enum{DREQ = 28};             // VS1053 Data request, ideally an Interrupt pin

        // 試作基板用(Arduino MEGA基板使用)
        // enum{BREAKOUT_RESET = 4};   // VS1053 reset pin (output)
        // enum{BREAKOUT_CS = 2};      // VS1053 chip select pin (output)
        // enum{BREAKOUT_DCS = 3};     // VS1053 Data/command select pin (output)
        // enum{CARDCS = 5};           // Card chip select pin
        // enum{DREQ = 1};             // VS1053 Data request, ideally an Interrupt pin

        enum{   // 割込みハンドラ内で実行するコマンドの種別を表す
            MSG_STOP   = 1,
            MSG_VOLUME = 2,
            MSG_BASS   = 3
        };
        static const uint16_t   VOLUME_MAP[VOLUME_MAX+1];
        static Adafruit_VS1053_FilePlayer  m_player;
        uint16_t m_volume;
        uint16_t m_bass;
        uint16_t m_treble;
        PlayerTimeCounter m_time_counter;
        CommandFIFO m_timer_queue;  // タイマ割込みハンドラへの通知用
        CommandFIFO m_ui_queue;     // ユーザインタフェースへの通知用
        static void onTimer();
        void loadConfig();

    public:
        MusicPlayer();
        void begin();
        void saveConfig();
        bool isStopped(){ return m_player.stopped(); }
        bool isPaused(){ return m_player.paused(); }
        uint32_t getElapsed();
        void setVolume(uint16_t vol);
        void setBass(uint16_t bass);
        void setTreble(uint16_t treble);
        void setVolumeDelta(int delta);
        void setBassDelta(int delta);
        void setTrebleDelta(int delta);
        uint16_t getVolume(){ return m_volume; }
        uint16_t getBass(){ return m_bass; }
        uint16_t getTreble(){ return m_treble; }
        void pause(bool pause);
        void stop(bool wait_for=false);
        bool play(const char *filename);
        bool trackEnded();
};

MusicPlayer& Player();

#endif
