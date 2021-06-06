#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include "SSD1322.h"
#include "player.h"
#include "playlist.h"
#include "ir_remote.h"
#include "spectrum_analyzer.h"

#define NUM_VIEWS   8   // 余裕を持った値だが、ビューを追加するときはこの値を超えていないかチェック

// -----------------------------------------------------------------------------
class View
{
    friend class PopupView;
    private:
        bool m_visible;
        static Vector<View *> m_views;
    protected:
        SSD1322 *m_oled;
        Playlist *m_playlist;
        uint8_t  m_id;
        virtual void show();
        virtual void hide();
        virtual void refresh();
    public:
        View(SSD1322 *oled, Playlist *playlist, uint8_t id);
        virtual void init();
        bool isVisible(){ return m_visible; }
        virtual void update();
        virtual void invalidate(bool partial);
        virtual bool handleIRR(IRRCODE code){ return false; }

        static View *getView(uint8_t id);
        static View *getActiveView();
        static void show(uint8_t id);
};

// -----------------------------------------------------------------------------
// class SleepView : public View
// {
//     public:
//         enum{ID = 99};
//         SleepView(SSD1322 *oled, Playlist *playlist);
// };

// -----------------------------------------------------------------------------
class PlaybackView : public View
{
    private:
        enum{MSGEQ7_STROBE = 2};
        enum{MSGEQ7_RESET = 3};

        enum{STOP = 0, PLAY = 1, PAUSE = 2};
        ImageList  m_state_images;
        ImageList  m_digit_images;
        ImageList  m_label_images;
        ImageList  m_icon_images;
        uint8_t    m_state;
        uint8_t    m_track_number[2];    // トラックNo.（十の位、一の位）
        uint8_t    m_elapsed_time[5];    // 演奏時間（MM:SS）
        ScrollText m_scroll_text;

        SpectrumAnalyzer *m_analyzer;
        uint8_t m_spectrum[SpectrumAnalyzer::NUM_CHANNELS];     // 各バンドのスペアナ表示値(0～16)
        uint8_t m_spectrum_gain;

        void setScrollText();
        void drawState(bool force_redraw);
        void drawTrackNumber(bool force_redraw);
        void drawElapsedTime(bool force_redraw);
        void drawSpectrum(bool force_redraw);
        void drawAlbumInfo(bool force_redraw);
        void drawTrackInfo(int row = -1);  //bool force_redraw);
    
    protected:
        void refresh();

    public:
        enum{ID = 1};
        PlaybackView(SSD1322 *oled, Playlist *playlist, SpectrumAnalyzer *analyzer);
        void init();
        void update();
        void invalidate(bool partial);
        bool handleIRR(IRRCODE code);
};

// -----------------------------------------------------------------------------
class ListView : public View
{
    protected:
        enum{ITEMS_PER_PAGE = 3};
        uint16_t m_item_count;
        uint16_t m_page_count;
        uint16_t m_page_pos;
        uint16_t m_cursor_pos;
        void show();
        void refresh();
        uint16_t cursorPosToIndex(uint16_t pos){
            return m_page_pos * ITEMS_PER_PAGE + pos; 
        }
        void getItemRect(uint16_t index, Rectangle& rc){ //int16_t& x, int16_t& y, int16_t& w, int16_t& h){
            rc.left = 0;
            rc.top  = 16 + (index % ITEMS_PER_PAGE)*16;
            rc.width = 256;
            rc.height = 16;
        }
        virtual uint16_t getItemCount(){ return 0; }
        virtual uint16_t getInitialSelection(){ return 0; }
        virtual void drawHeader(){}
        virtual void drawIndex();
        virtual void drawItem(uint16_t index, Rectangle& rc, bool selected);

    private:
        static ImageList m_cursor_images;
        void drawListItems();

    public:
        ListView(SSD1322 *oled, Playlist *playlist, uint8_t id);
        void init();
        void moveCursor(int direction);
        void movePage(int direction);
        uint16_t getSelectedIndex(){ return cursorPosToIndex(m_cursor_pos); }
        bool handleIRR(IRRCODE code);
};

// -----------------------------------------------------------------------------
class AlbumListView : public ListView
{
    private:
        Artist *m_artist;

    protected:
        uint16_t getItemCount();
        uint16_t getInitialSelection();
        void drawHeader();
        void drawItem(uint16_t index, Rectangle& rc, bool selected);

    public:
        enum{ID = 2};
        AlbumListView(SSD1322 *oled, Playlist *playlist);
        void setArtist(Artist *artist){ m_artist = artist; }
        Album *getSelection();
        bool handleIRR(IRRCODE code);
};

// -----------------------------------------------------------------------------
class ArtistListView : public ListView
{
    private:
        Artist *m_artist;

    protected:
        uint16_t getItemCount();
        uint16_t getInitialSelection();
        void drawHeader();
        void drawItem(uint16_t index, Rectangle& rc, bool selected);

    public:
        enum{ID = 3};
        ArtistListView(SSD1322 *oled, Playlist *playlist);
        void setArtist(Artist *artist){ m_artist = artist; }
        Artist *getSelection();
        bool handleIRR(IRRCODE code);
};

// -----------------------------------------------------------------------------
class SSLine
{
    private:
        int16_t  m_x[2], m_y[2];
        int16_t  m_dx[2], m_dy[2];
    public:
        SSLine();
        SSLine& operator = (SSLine& src);
        void move();
        void draw(SSD1322 *oled, uint16_t color);
        void getBound(Rectangle& rc){
            rc.left = min(m_x[0], m_x[1]);
            rc.top  = min(m_y[0], m_y[1]);
            rc.width  = abs(m_x[0] - m_x[1]) + 1;
            rc.height = abs(m_y[0] - m_y[1]) + 1;
        }
};

// -----------------------------------------------------------------------------
class ScreenSaverView : public View
{
    private:
        enum{MAX_LINE_NUM = 20};
        enum{NUM_COLORS = 22};
        // enum{IDLE_TIMEOUT = 300000};
        enum{UPDATE_INTERVAL = 40};
        SSLine m_lines[MAX_LINE_NUM];
        uint32_t m_idle_timeout;
        int m_line_count;
        uint32_t m_next_tick;
        int m_color_index;

        static const uint16_t COLOR_TABLE[NUM_COLORS];

    protected:
        void refresh();

    public:
        enum{ID = 4};
        ScreenSaverView(SSD1322 *oled, Playlist *playlist);
        void init();
        void update();
        bool handleIRR(IRRCODE code);
        uint32_t getTimeoutValue(){ return m_idle_timeout; }
        void setTimeoutValue(uint32_t t);
};

// -----------------------------------------------------------------------------
class ConfigView : public View
{
    private:
        static const int16_t EDITCHAR_POS_X[2][12];
        static const int16_t EDITCHAR_POS_Y[2];
        static const int EDIT_LENGTH[2];
        enum{EDITCHAR_WIDTH = 7};
        enum{EDITCHAR_HEIGHT = 16};
        
        char m_edit_value[2][20];
        char m_clock[20];
        int  m_edit_col;
        int  m_edit_row;
        ImageList m_cursor_images;

        void drawClock(bool force_redraw);
        void drawScreenSaverTime(bool force_redraw);
        void startEditing();
        void cancelEditing();
        void input(int digit);
        void acceptEditing();

    protected:
        void refresh();

    public:
        enum{ID = 5};
        ConfigView(SSD1322 *oled, Playlist *playlist);
        void init();
        void update();
        bool handleIRR(IRRCODE code);
};

// -----------------------------------------------------------------------------
class ClockView : public View
{
    private:
        ImageList m_sseg_images;
        ImageList m_digit_images;
        ImageList m_week_images;
        uint16_t  m_month;
        uint16_t  m_day;
        uint16_t  m_hour;
        uint16_t  m_minute;
        uint32_t  m_tick;
        uint32_t  m_timeout;
        bool      m_blink;

        void drawClock(uint16_t hour, uint16_t minute, bool blink);
        void drawCalendar(uint16_t month, uint16_t day, uint16_t week);

    protected:
        void show();
        void refresh();

    public:
        enum{ID = 6};
        ClockView(SSD1322 *oled, Playlist *playlist);
        void init();
        void update();
        bool handleIRR(IRRCODE code);
};

// -----------------------------------------------------------------------------
class PopupView
{
    private:
        enum{
            PROP_VOLUME,
            PROP_BASS,
            PROP_TREBLE
        };
        enum{IDLE_TIMEOUT = 5000};
        SSD1322 *m_oled;
        bool m_visible;
        uint8_t m_prop_type;
        uint32_t m_idle_timer;
        Rectangle m_popup_rect;
        ImageList m_label_images;
        void refresh();
        void drawBar(bool refresh);
    public:
        PopupView(SSD1322 *oled);
        void init();
        void show(uint8_t prop);
        void hide();
        void update();
        bool handleIRR(IRRCODE code);
        bool isVisible(){ return m_visible; }
};

#endif
