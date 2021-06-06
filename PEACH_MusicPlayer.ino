#include <Arduino.h>
#include <Wire.h>
#include <SD.h>
#include "SSD1322.h"
#include "player.h"
#include "playlist.h"
#include "ir_remote.h"
#include "oled_display.h"
#include "spectrum_analyzer.h"
#include "M41T62.h"

// 試作基板（Arduino MEGA用基板使用）
// #define OLED_CS         26
// #define OLED_RES        27
// #define OLED_E          31
// #define OLED_RW         32
// #define OLED_DC         33
// #define MSGEQ7_STROBE   2
// #define MSGEQ7_RESET    3
// #define MSGEQ7_DATA_OUT A0

// 本番基板
#define OLED_CS         70
#define OLED_RES        69
#define OLED_E          22
#define OLED_RW         23
#define OLED_DC         24
#define MSGEQ7_STROBE   2
#define MSGEQ7_RESET    3

#define IDLE_TIMEOUT    60000

SSD1322 g_oled(OLED_CS, OLED_DC, OLED_RES, OLED_E, OLED_RW);
IRRemote  g_irr;
SpectrumAnalyzer g_analyzer;
Playlist g_playlist;
bool g_power_on;

PlaybackView   playback_view(&g_oled, &g_playlist, &g_analyzer);
AlbumListView  album_list_view(&g_oled, &g_playlist);
ArtistListView artist_list_view(&g_oled, &g_playlist);
ScreenSaverView screensave_view(&g_oled, &g_playlist);
ClockView clock_view(&g_oled, &g_playlist);
ConfigView config_view(&g_oled, &g_playlist);

PopupView g_popup(&g_oled);

// -----------------------------------------------------------------------------
bool controlAudio(IRRCODE code)
{
    Artist *artist = g_playlist.getSelectedArtist();
    Album *album = artist->getSelectedAlbum();

    switch( code )
    {
        // case IRRemote::POWER:
        //     togglePower();
        //     break;            
        case IRRemote::PLAY_PAUSE:
            if( Player().isStopped() )
            {
                Serial.println("play");
                const char *filename = album->getCurrentSong()->getFileName();
                Serial.println(filename);
                Player().play(filename);
            }
            else
            {
                bool paused = Player().isPaused();
                if( paused )
                {
                    Serial.println("resume");
                    Player().pause(false);
                }
                else
                {
                    Serial.println("pause");
                    Player().pause(true);
                }
            }
            break;
        case IRRemote::FN_STOP:
            Serial.println("stop");
            Player().stop(true);
            album->seekFirst();
            break;
        case IRRemote::PREV:
            if( !Player().isStopped() )
            {
                Serial.println("prev");
                Player().stop(true);
                album->seekPrev();
                const char *filename = album->getCurrentSong()->getFileName();
                Serial.println(filename);
                Player().play(filename);
            }
            else
            {
                return false;
            }
            break;
        case IRRemote::NEXT:
            if( !Player().isStopped() && album->hasNext() )
            {
                Serial.println("next");
                Player().stop(true);
                album->seekNext();
                const char *filename = album->getCurrentSong()->getFileName();
                Serial.println(filename);
                Player().play(filename);
            }
            else
            {
                return false;
            }
            break;
        default:
            return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
void controlLED()
{
    static int f = 0;
    static uint32_t t = 0;

    uint32_t now = millis();

    if( Player().isStopped() )
    {
        digitalWrite(PIN_LED_BLUE, LOW);
        if( now - t >= 500 )
        {
            f = 1 - f;
            t = now;
            digitalWrite(PIN_LED_RED, f);
        }
    }
    else
    {
        digitalWrite(PIN_LED_RED, LOW);
        if( Player().isPaused() )
        {
            digitalWrite(PIN_LED_BLUE, HIGH);
        }
        else
        {
            if( now - t >= 500 )
            {
                f = 1 - f;
                t = now;
                digitalWrite(PIN_LED_BLUE, f);
            }
        }
    }
}

// -----------------------------------------------------------------------------
void setup()
{
    Serial.begin(9600);
    Wire.begin();
    SD.begin();
    g_irr.begin();
    g_analyzer.begin(MSGEQ7_RESET, MSGEQ7_STROBE);

    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_RED, HIGH);
    digitalWrite(PIN_LED_BLUE, LOW);

    g_oled.init();
    g_oled.displayOn();
    g_playlist.load("playlist.dat");
    Player().begin();

    View::getView(PlaybackView::ID)->init();
    View::getView(ArtistListView::ID)->init();
    View::getView(AlbumListView::ID)->init();
    View::getView(ScreenSaverView::ID)->init();
    View::getView(ClockView::ID)->init();
    View::getView(ConfigView::ID)->init();
    g_popup.init();
    View::show(PlaybackView::ID);
}

// -----------------------------------------------------------------------------
void loop()
{
    IRRCODE code = g_irr.read();
    if( code )
    {
        if( !g_popup.handleIRR(code) )
        {
            if( controlAudio(code) )
            {
                View::show(PlaybackView::ID);
            }
            else
            {
                View::getActiveView()->handleIRR(code);
            }
        }
    }

    // 無操作タイムアウトの監視のため、リモコン受信有無に関わらず必ずScreenSaverViewに制御の機会を与える
    View::getView(ScreenSaverView::ID)->handleIRR(code);

    if( Player().trackEnded() )
    {
        Serial.println("track ended");
        Artist *artist = g_playlist.getSelectedArtist();
        Album *album = artist->getSelectedAlbum();
        if( album->hasNext() )
        {
            album->seekNext();
            const char *filename = album->getCurrentSong()->getFileName();
            Serial.println(filename);
            Player().play(filename);
        }
        if( View::getView(PlaybackView::ID)->isVisible() )
        {
            View::getView(PlaybackView::ID)->invalidate(false);
        }
    }
    g_popup.update();
    View::getActiveView()->update();
    controlLED();
}

