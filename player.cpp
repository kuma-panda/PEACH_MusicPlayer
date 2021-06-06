#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <MsTimer2.h>
#include "VS1053.h"
#include "player.h"
#include "eeprom_24lc.h"

// -----------------------------------------------------------------------------
MusicPlayer& Player()
{
    static MusicPlayer _player;
    return _player;
}

// -----------------------------------------------------------------------------
void MusicPlayer::onTimer()
{
    MusicPlayer& player = Player();
    if( !player.isStopped() )
    {
        player.m_player.feedBuffer();
        if( player.isStopped() )
        {
            player.m_time_counter.reset();
            player.m_ui_queue.push(MSG_STOP);
        }
    }

    uint8_t c = player.m_timer_queue.pop();
    uint16_t v;
    switch( c )
    {
        case MSG_STOP:
            player.m_player.stopPlaying();
            break;
        case MSG_VOLUME:
            v = VOLUME_MAP[Player().m_volume];
            player.m_player.setVolume(v, v);
            break;
        case MSG_BASS:
            player.m_player.setBass(player.m_bass, player.m_treble);
            break;
    }
    if( player.isStopped() && player.m_time_counter.isActive() )
    {
        player.m_time_counter.reset();
    }
}

// -----------------------------------------------------------------------------
const uint16_t MusicPlayer::VOLUME_MAP[MusicPlayer::VOLUME_MAX+1] = 
{
    // VS1053へ設定する音量の値。
    // setVolume() に指定する値（= m_volume の値）はこの配列のインデックスで
    // あることに注意(0～VOLUME_MAX)。
    65535,  // ミュート 
    52, 
    40, 
    33, 
    28,     // デフォルト 
    24, 
    21, 
    18, 
    16, 
    14, 
    12, 
    10, 
    9, 
    7, 
    6, 
    5, 
    4, 
    3, 
    2, 
    1, 
    0       // 最大音量
};

Adafruit_VS1053_FilePlayer MusicPlayer::m_player(
    MusicPlayer::BREAKOUT_RESET, 
    MusicPlayer::BREAKOUT_CS, 
    MusicPlayer::BREAKOUT_DCS, 
    MusicPlayer::DREQ, 
    MusicPlayer::CARDCS
);

// -----------------------------------------------------------------------------
MusicPlayer::MusicPlayer() : m_volume(0), m_bass(0), m_treble(0)
{

}

// -----------------------------------------------------------------------------
void MusicPlayer::begin()
{
    Serial.begin(9600);
    if( !m_player.begin() ) 
    { 
        // initialise the music player
        Serial.println("Couldn't find VS1053, do you have the right pins defined?");
        while(1);
    }
    Serial.println("VS1053 found");

    setVolume(DEFAULT_VOLUME_VALUE);
    setBass(0);
    setTreble(0);
    loadConfig();

    MsTimer2::set(1, MusicPlayer::onTimer);
    MsTimer2::start();
}

// -----------------------------------------------------------------------------
void MusicPlayer::loadConfig()
{
    // +100     : 音量
    // +101     : BASS
    // +102     : TREBLE
    // +103     : チェックサム
    uint8_t config[4];
    EEPROM().read(0x100, 4, config);
    uint8_t sum = 0;
    for( int i = 0 ; i < 4 ; i++ )
    {
        sum += config[i];
    }
    if( sum != 0xFF )
    {
        Serial.println("EEPROM Read Error (0x100 - 0x103) -- checksum unmatched.");
        return;
    }
    setVolume(config[0]);
    setBass(config[1]);
    setTreble(config[2]);
}

// -----------------------------------------------------------------------------
void MusicPlayer::saveConfig()
{
    uint8_t config[4];
    config[0] = (uint8_t)getVolume();
    config[1] = (uint8_t)getBass();
    config[2] = (uint8_t)getTreble();
    uint8_t sum = config[0] + config[1] + config[2];
    config[3] = 0xFF - sum;
    EEPROM().write(0x100, 4, config);
}


//------------------------------------------------------------------------------
//   音量調節
//------------------------------------------------------------------------------
void MusicPlayer::setVolume(uint16_t vol)
{
    if( vol > VOLUME_MAX )
    {
        vol = VOLUME_MAX;
    }
    m_volume = vol;
    vol = VOLUME_MAP[vol];
    Serial.print("set volume to ");
    Serial.println(vol);
    m_timer_queue.push(MSG_VOLUME);
    // m_player.setVolume(vol, vol);
}
void MusicPlayer::setVolumeDelta(int delta)
{
    if( delta < 0 && m_volume > 0 )
    {
        setVolume(m_volume-1);
    }
    else if( delta > 0 && m_volume < VOLUME_MAX-1 )
    {
        setVolume(m_volume+1);
    }
}

//------------------------------------------------------------------------------
//   低音域の増幅率設定
//   bass: ゲイン(dB, 0～BASS_MAX)
//         0 で増幅なし，0以外の値を指定すると 20Hz以下の音域が指定したゲインで
//         増幅される
//------------------------------------------------------------------------------
void MusicPlayer::setBass(uint16_t bass)
{
     if( bass > BASS_MAX )
     {
          bass = BASS_MAX;
     }
     m_bass = bass;
     Serial.print("set bass gain to ");
     Serial.println(bass);
     m_timer_queue.push(MSG_BASS);
    //  m_player.setBass(m_bass, m_treble);
}
void MusicPlayer::setBassDelta(int delta)
{
    if( delta < 0 && m_bass > 0 )
    {
        setBass(m_bass-1);
    }
    else if( delta > 0 && m_bass < BASS_MAX-1 )
    {
        setBass(m_bass+1);
    }
}

//------------------------------------------------------------------------------
//   高音域の増幅率設定
//   treble: ゲイン(db, 0～TREBLE_MAX)
//         0 で増幅なし，0以外の値を指定すると 10kHz以上の音域が指定したゲインで
//         増幅される
//------------------------------------------------------------------------------
void MusicPlayer::setTreble(uint16_t treble)
{
     if( treble > TREBLE_MAX )
     {
          treble = TREBLE_MAX;
     }
     m_treble = treble;
     Serial.print("set treble gain to ");
     Serial.println(treble);
     m_timer_queue.push(MSG_BASS);
    //  m_player.setBass(m_bass, m_treble);
}
void MusicPlayer::setTrebleDelta(int delta)
{
    if( delta < 0 && m_treble > 0 )
    {
        setTreble(m_treble-1);
    }
    else if( delta > 0 && m_treble < TREBLE_MAX-1 )
    {
        setTreble(m_treble+1);
    }
}

// -----------------------------------------------------------------------------
uint32_t MusicPlayer::getElapsed()
{
    return m_time_counter.getValue();
}

// -----------------------------------------------------------------------------
void MusicPlayer::pause(bool pause)
{
    m_player.pausePlaying(pause);
    if( pause )
    {
        m_time_counter.stop();
    }
    else
    {
        m_time_counter.start();
    }
}

// -----------------------------------------------------------------------------
void MusicPlayer::stop(bool wait_for)
{
    m_timer_queue.push(MSG_STOP);
    if( wait_for )
    {
        while( !isStopped() ){}
        delay(50);
    }
}

// -----------------------------------------------------------------------------
//  指定した曲を再生する
//  曲の再生が終了（または停止させる）までこの関数からは戻らないことに注意
//  再生中は setPlayingProc() で指定した関数が定期的に呼び出される
// -----------------------------------------------------------------------------
bool MusicPlayer::play(const char *filename)
{
    if( !m_player.stopped() )
    {
        return false;
    }

    if( !m_player.startPlayingFile(filename) )
    {
        return false;
    }

    m_time_counter.start();
    
    // while( m_player.playingMusic ) 
    // {
    //     // twiddle thumbs
    //     m_player.feedBuffer();
    //     if( m_pfnProc )
    //     {
    //         m_pfnProc();
    //     }
    // }

    // m_time_counter.reset();

    // // music file finished!
    return true;
}

bool MusicPlayer::trackEnded()
{
    uint8_t c = m_ui_queue.pop();
    return c == MSG_STOP;
}
