#ifndef SSD1322_H
#define SSD1322_H

#include <Arduino.h>
#include <SD.h>

// -----------------------------------------------------------------------------
class Glyph
{
    private:
        uint16_t m_code;
        int16_t  m_width;
        int16_t  m_height;
        int16_t  m_offset;
        int16_t  m_advance;
        // bool     m_anti_alias;
        uint16_t *m_data;

    public:
        enum
        {
            SMALL = 0,
            LARGE = 1,
            TINY  = 2
        };
        // static const int16_t HEIGHT[2];
        // enum{HEIGHT = 16};
        Glyph();
        void load(File f, int16_t height);
        uint16_t getCode(){ return m_code; }
        int16_t  getWidth(){ return m_width; }
        int16_t  getHeight(){ return m_height; }
        int16_t  getOffset(){ return m_offset; }
        int16_t  getAdvance(){ return m_advance; }
        uint16_t *getData(){ return m_data; }
        uint8_t  getPixel(int16_t x, int16_t y);
};

// -----------------------------------------------------------------------------
class Font
{
    private:
        enum{MAX_GLYPH_NUM = 1023};
        Glyph  m_glyph[MAX_GLYPH_NUM];
        Glyph *m_hash_table[MAX_GLYPH_NUM];
        // bool m_anti_alias;
        void insert_to_hash(Glyph *g);
    public:
        Font();
        void load(const char *path, int16_t height); //, bool anti_alias=false);
        Glyph *getGlyph(uint16_t code);
        // bool isAntiAlias(){ return m_anti_alias; }
        static char *getCharCodeAt(char *p, uint16_t *code);
};

// -----------------------------------------------------------------------------
class Image
{
    private:
        int16_t m_width;
        int16_t m_height;
        uint8_t *m_data;
    public:
        Image();
        void load(File f);
        int16_t getWidth(){ return m_width; }
        int16_t getHeight(){ return m_height; }
        uint8_t getPixel(int16_t x, int16_t y);
};

// -----------------------------------------------------------------------------
class ImageList
{
    private:
        Image *m_images;
        int16_t m_count;
    public:
        ImageList();
        void load(const char *path, int16_t count);
        int16_t getCount(){ return m_count; }
        int16_t getImageWidth();
        int16_t getImageHeight();
        Image *getImage(int16_t index);
};

// -----------------------------------------------------------------------------
#define SSD1322_SEG_COUNT 480

#define SSD1322_WHITE 0x0F
#define SSD1322_BLACK 0x00

#define SSD1322_DEPTH 4  // 4 bits per pixel

enum SSD1322_Mode
{
    SSD1322_MODE_NORMAL,
    SSD1322_MODE_ALL_ON,
    SSD1322_MODE_ALL_OFF,
    SSD1322_MODE_INVERT
};

// NOTE: Section 10.1.9 of the data sheet mixes up 0xA4 and 0xA6.
#define SSD1322_CMD_ENGREYSCALE   0x00
#define SSD1322_CMD_COL_ADDRESS   0x15
#define SSD1322_CMD_WRITE_RAM     0x5C
#define SSD1322_CMD_ROW_ADDRESS   0x75
#define SSD1322_CMD_MUX_RATIO     0xCA
#define SSD1322_CMD_REMAP         0xA0
#define SSD1322_CMD_START_LINE    0xA1
#define SSD1322_CMD_OFFSET_LINE   0xA2
#define SSD1322_CMD_MODE_ALL_OFF  0xA4
#define SSD1322_CMD_MODE_ALL_ON   0xA5
#define SSD1322_CMD_MODE_NORMAL   0xA6
#define SSD1322_CMD_MODE_INVERT   0xA7
#define SSD1322_CMD_VDDSEL        0xAB
#define SSD1322_CMD_DISPLAY_ON    0xAF
#define SSD1322_CMD_DISPLAY_OFF   0xAE
#define SSD1322_CMD_PHASELEN      0xB1
#define SSD1322_CMD_CLOCK_DIVIDER 0xB3
#define SSD1322_CMD_DISPENHA      0xB4
#define SSD1322_CMD_SECPRECHRG    0xB6
#define SSD1322_CMD_SETGRAYTABLE  0xB8
#define SSD1322_CMD_DEFGRYTABLE   0xB9
#define SSD1322_CMD_PRECHRGVOL    0xBB
#define SSD1322_CMD_SETVCOMH      0xBE
#define SSD1322_CMD_CONTRSTCUR    0xC1
#define SSD1322_CMD_MSTCONTRST	  0xC7
#define SSD1322_CMD_DISPENHB      0xD1
#define SSD1322_CMD_COMLOCK	      0xFD

// -----------------------------------------------------------------------------
class Rectangle
{
    public:
        int16_t left;
        int16_t top;
        int16_t width;
        int16_t height;
        Rectangle() : left(0), top(0), width(0), height(0){}
        Rectangle(int16_t l, int16_t t, int16_t w, int16_t h) : left(l), top(t), width(w), height(h){}
        void setRect(int16_t l, int16_t t, int16_t w, int16_t h){
            left = l;
            top = t;
            width = w;
            height = h;
        }
        Rectangle& operator = (Rectangle& rc){
            left = rc.left;
            top  = rc.top;
            width = rc.width;
            height = rc.height;
            return *this;
        }
        void inflate(int16_t dx, int16_t dy){
            left -= dx;
            top -= dy;
            width += dx*2;
            height += dy*2;
        }
        void unionWith(Rectangle& other){
            int16_t r = max(right(), other.right());
            int16_t b = max(bottom(), other.bottom());
            left = min(left, other.left);
            top  = min(top, other.top);
            width = r - left + 1;
            height = b - top + 1;            
        }

        void empty(){ width = height = 0; }
        bool isEmpty(){ return (width == 0) || (height == 0); }
        bool include(int16_t x, int16_t y){
            return (left <= x) && (x < (left+width)) && (top <= y) && (y < (top+height));
        }
        int16_t right(){ return left + width - 1; }
        int16_t bottom(){ return top + height - 1; }
};

// -----------------------------------------------------------------------------
class ScrollLineBuffer
{
    private:
        enum{BUFFER_SIZE = 1024};       // バッファの横幅(バイト単位、１バイトで横２ピクセル分)
        // m_buffer[0] : 偶数オリジンでニブル単位でピクセルデータを格納
        // m_buffer[1] : 奇数オリジンでニブル単位でピクセルデータを格納(先頭ピクセルは格納しない)
        uint8_t *m_buffer[2];
    public:
        ScrollLineBuffer(){
            m_buffer[0] = (uint8_t *)malloc(BUFFER_SIZE);
            m_buffer[1] = (uint8_t *)malloc(BUFFER_SIZE);
            clear();
        }
        void clear(){
            memset(m_buffer[0], 0x00, BUFFER_SIZE);
            memset(m_buffer[1], 0x00, BUFFER_SIZE);
        }
        void setPixel(int16_t x, uint8_t color){

            //           x   0   1   2   3   4   5   6   7
            // m_buffer[0] [HH  LL][HH  LL][HH  LL][HH  LL]...
            // m_buffer[0]     [HH  LL][HH  LL][HH  LL][HH ... 

            // x が偶数の場合は m_buffer[0][x/2]     の上位ニブルに書き込む
            //                  m_buffer[1][(x-1)/2] の下位ニブルに書き込む
            // x が奇数の場合は m_buffer[0][x/2]     の下位ニブルに書き込む
            //                  m_buffer[1][(x-1)/2] の上位ニブルに書き込む

            if( !color ){ return; }
            if( x % 2 )
            {
                m_buffer[0][x/2] |= (color & 0x0F);
                m_buffer[1][(x-1)/2] |= ((color << 4) & 0xF0);
            }
            else
            {
                m_buffer[0][x/2] |= ((color << 4) & 0xF0);
                if( x > 0 )
                {
                    m_buffer[1][(x-1)/2] |= (color & 0x0F);
                }
            }
        }
        uint8_t getPixel(int16_t x){
            return (m_buffer[0][x/2] & ((x % 2)? 0x0F : 0xF0))? 0x0F : 0x00; 
        }
        
        uint8_t *getDataPtr(int16_t x){
            if( x % 2 )
            {
                return &m_buffer[1][(x-1)/2];
            }
            else
            {
                return &m_buffer[0][x/2];
            }
        }
};

class ScrollText
{
    private:
        enum{SCROLL_INTERVAL = 40};
        enum{PAUSE_TIMEOUT = 4000};
        enum
        {
            STATE_STATIC   = 0,     // 上段、下段とも静止表示している状態
            STATE_SCROLL_1 = 1,     // 上段がスクロール表示中
            STATE_SCROLL_2 = 2      // 下段がスクロール表示中
        };

        int16_t   m_viewport_width;     // ビューポート（実際に画面に表示させる範囲）の幅（ピクセル単位。必ず偶数を指定する）
        ScrollLineBuffer m_buffer[32];  
        int16_t   m_text_width[2];      // テキストの描画サイズ([上段、下段])。これが m_viewport_width 以下であればスクロールは機能させず、常時静的表示のみとなる
        int       m_scroll_state;       // STATE_??? のいずれか
        uint32_t  m_next_tick;          // 次に描画する時刻(msec)
        int       m_offset;             // スクロール表示中の領域で、次に描画する際の先頭(左端)位置

        bool needScroll(int row){
            return m_text_width[row] > m_viewport_width;
        }
        bool isScrolling(int row){
            return ((row == 0) && (m_scroll_state == STATE_SCROLL_1)) || ((row == 1) && (m_scroll_state == STATE_SCROLL_2)); 
        }

    public:
        enum{VIEWPORT_HEIGHT = 16};
        ScrollText();
        void init(int16_t width);
        void setText(int row, const char *text, Font *font);
        int16_t getViewportWidth(){ return m_viewport_width; }
        void startScroll();
        int updateScroll();
        // uint8_t getPixel(int row, int16_t dx, int16_t dy);
        uint8_t *getLineData(int row, int16_t dy);

};

// -----------------------------------------------------------------------------
class SSD1322
{
    private:
        int m_width;   // Display width
        int m_height;  // Display height

        int m_csPin;
        int m_dcPin;
        int m_resetPin;
        int m_ePin;
        int m_rwPin;

        uint8_t *m_buf;
        int m_stride;

        int m_startCol;  // First displayed column of each row
        int m_endCol;    // Last displayed column of each row

        Font m_font[3];

        Rectangle m_popup_rect;
        // bool m_popup_visible;
        bool m_popup_drawing;

        void writeCmd(uint8_t c);
        void writeData(uint8_t d);

        void reset(void);

        bool initFrameBuffer(void);

        void setMuxRatio(int h);
        void setRowRange(int r1, int r2);
        void setColumnRange(int c1, int c2);
        void setClock(int freq, int divisor);
        void setRemap(uint8_t a, uint8_t b);
        void setGrayscaleTable();
        void setStartRow(int r);
        void setOffsetRow(int r);
        uint8_t getPixel(int16_t x, int16_t y);
        bool getPopupRectAddress(int16_t y, uint32_t& start, uint32_t& end);

    public:
        enum{FONT_SMALL = 0, FONT_LARGE = 1, FONT_TINY = 2};
        SSD1322(int csPin, int dcPin, int resetPin, int ePin, int rwPin);
        bool init(void);
        void displayOn(void);
        void displayOff(void);
        void setDisplayMode(SSD1322_Mode mode);

        Font *getFont(uint8_t size){ return &m_font[size]; }

        void drawPixel(int16_t x, int16_t y, uint16_t color);
        void drawHzLine(int16_t x, int16_t y, int16_t len, uint16_t color);
        void drawVtLine(int16_t x, int16_t y, int16_t len, uint16_t color);
        void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        void drawRect(Rectangle& rc, uint16_t color);
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        void fillRect(Rectangle& rc, uint16_t color);
        int16_t drawChar(int16_t x, int16_t y, uint16_t code, uint8_t size, uint8_t color);
        int16_t drawASCIIText(int16_t x, int16_t y, const char *text, uint8_t size, uint8_t color);
        int16_t drawString(int16_t x, int16_t y, const char *text, uint8_t size, uint8_t color);
        int16_t getTextWidth(const char *text, uint8_t size);
        void drawImage(int16_t x, int16_t y, Image *image);
        void drawScrollText(int row, int16_t x, int16_t y, ScrollText *text);
        // void scroll(Rectangle& rect, uint8_t *fill_data, bool refresh);
        void clear(uint8_t color);
        void display(void);
        void invalidateRect(int16_t left, int16_t top, int16_t w, int16_t h);
        void invalidateRect(Rectangle& rc);

        void enablePopup(Rectangle& rc);
        void disablePopup();
        void startDrawPopup();
        void endDrawPopup();

        // void invalidate(){ invalidateRect(0, 0, m_width, m_height); }
};

#endif
