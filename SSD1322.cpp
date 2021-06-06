#include <Arduino.h>
#include "SSD1322.h"

////////////////////////////////////////////////////////////////////////////////
//  Glyph
////////////////////////////////////////////////////////////////////////////////
Glyph::Glyph() : m_code(0), m_width(0), m_height(0), m_offset(0), m_advance(0), 
    m_data(NULL) //, m_anti_alias(false)
{
}

// -----------------------------------------------------------------------------
void Glyph::load(File f, int16_t height) //, bool anti_alias)
{
    // m_anti_alias = anti_alias;
    m_height = height;
    // フォントファイルから読み込む
    // uint8_t buffer[16*32];
    // uint16_t glyph_size;
    f.read(&m_code, sizeof(m_code));
    f.read(&m_width, sizeof(m_width));
    f.read(&m_offset, sizeof(m_offset));
    f.read(&m_advance, sizeof(m_advance));
    if( m_width > 0 )
    {
        m_data = (uint16_t *)malloc(2*m_width);
        f.read(m_data, 2*m_width);
    }
}

// -----------------------------------------------------------------------------
uint8_t Glyph::getPixel(int16_t x, int16_t y)
{
    if( !m_data || x >= m_width || y >= m_height )
    {
        return 0x00;
    }
    return (m_data[x] & (0x0001 << y))? 0x0F : 0x00;
}

////////////////////////////////////////////////////////////////////////////////
//  Font
////////////////////////////////////////////////////////////////////////////////
Font::Font()
{
    memset(m_hash_table, 0, sizeof(m_hash_table));
}

// -----------------------------------------------------------------------------
void Font::load(const char *path, int16_t height)
{
    // m_anti_alias = anti_alias;
    File f = SD.open(path);
    if( !f )
    {
        Serial.print("Cannot open ");
        Serial.println(path);
        while( true ){}
    }    
    uint16_t glyph_num;
    f.read(&glyph_num, 2);
    for( uint16_t i = 0 ; i < glyph_num ; i++ )
    {
        m_glyph[i].load(f, height); //, anti_alias);
        insert_to_hash(&m_glyph[i]);
    }
    f.close();
    Serial.print(path);
    Serial.print(" successfully loaded. (");
    Serial.print(glyph_num, DEC);
    Serial.println(" glyphs)");
}

// -----------------------------------------------------------------------------
void Font::insert_to_hash(Glyph *g)
{
    int index = g->getCode() % MAX_GLYPH_NUM;
    while( m_hash_table[index] )
    {
        //go to next cell
        index = (index + 1) % MAX_GLYPH_NUM;
    }
    m_hash_table[index] = g;
}

// -----------------------------------------------------------------------------
Glyph *Font::getGlyph(uint16_t code)
{
    int index = code % MAX_GLYPH_NUM;
    while( m_hash_table[index] )
    {
        if( m_hash_table[index]->getCode() == code )
        {
            return m_hash_table[index];
        }
        index = (index + 1) % MAX_GLYPH_NUM;
    }
    return NULL;
}

//------------------------------------------------------------------------------
//   UTF-8バイト列で，pの指す文字の文字コード(UCS-2)を取得する
//   取得した文字コードは *code に格納され，消費したバイト数ぶん進めたポインタを返す
//------------------------------------------------------------------------------
char *Font::getCharCodeAt(char *p, uint16_t *code)
{
    if( *p == 0 )
    {
        *code = 0x0000;
        return p;
    }
    if( (*p & 0xF0) == 0xE0 )
    {
        *code = (((uint16_t)(*p & 0x0F))<<12) | (((uint16_t)(*(p+1) & 0x3F))<<6) | ((uint16_t)(*(p+2) & 0x3F));
    //   if( *code == 0xFF5E ){ *code = 0x301C; }     // '～' の誤変換対策
        return p+3;
    }
    if( (*p & 0xE0) == 0xC0 )
    {
        *code = (((uint16_t)(*p & 0x1F))<<6) | ((uint16_t)(*(p+1) & 0x3F));
        return p+2;
    }
    else if( 0x20 <= *p && *p <= 0x7E )
    {
        *code = (uint16_t)*p;
        return p+1;
    }
    *code = 0x0000;
    return p+1;
}



////////////////////////////////////////////////////////////////////////////////
//  Image
////////////////////////////////////////////////////////////////////////////////
Image::Image() : m_width(0), m_height(0), m_data(NULL)
{

}

// -----------------------------------------------------------------------------
void Image::load(File f)
{
    f.read(&m_width, sizeof(m_width));
    f.read(&m_height, sizeof(m_height));
    int16_t size = (m_width / 2) * m_height;
    m_data = (uint8_t *)malloc(size);
    f.read(m_data, size);
}

// -----------------------------------------------------------------------------
uint8_t Image::getPixel(int16_t x, int16_t y)
{
    if( x < 0 || m_width <= x || y < 0 || m_height <= y )
    {
        return 0x00;
    }
    uint16_t addr = y * (m_width / 2) + (x / 2);
    if( x % 2 )
    {
        return m_data[addr] & 0x0F;
    }
    else
    {
        return (m_data[addr] >> 4) & 0x0F;
    }
}

////////////////////////////////////////////////////////////////////////////////
//  ImageList
////////////////////////////////////////////////////////////////////////////////
ImageList::ImageList() : m_images(NULL), m_count(0)
{

}

// -----------------------------------------------------------------------------
void ImageList::load(const char *path, int16_t count)
{
    File f = SD.open(path);
    if( !f )
    {
        Serial.print("Cannot open ");
        Serial.println(path);
        while( true ){}
    }    
    m_images = new Image[count];
    m_count = count;
    for( int16_t n = 0 ; n < count ; n++ )
    {
        m_images[n].load(f);
    }
    f.close();
    Serial.print(path);
    Serial.println(" successfully loaded");
}

// -----------------------------------------------------------------------------
int16_t ImageList::getImageWidth()
{
    if( m_images )
    {
        return m_images[0].getWidth();
    }
    return 0;
}

// -----------------------------------------------------------------------------
int16_t ImageList::getImageHeight()
{
    if( m_images )
    {
        return m_images[0].getHeight();
    }
    return 0;
}

// -----------------------------------------------------------------------------
Image *ImageList::getImage(int16_t index)
{
    if( m_images && index < m_count )
    {
        return &m_images[index];
    }
    Serial.println("ImageList index out of range");
    while( true ){}
}

////////////////////////////////////////////////////////////////////////////////
//  ScrollText
////////////////////////////////////////////////////////////////////////////////
ScrollText::ScrollText() : m_viewport_width(256), m_scroll_state(STATE_STATIC), m_offset(0)
{
    m_text_width[0] = 0;
    m_text_width[0] = 0;
}

// -----------------------------------------------------------------------------
void ScrollText::init(int16_t width)
{
    m_viewport_width = 2*(width/2); // 必ず偶数をセット
    m_text_width[0] = 0;
    m_text_width[1] = 0;
    m_scroll_state = STATE_STATIC;
    m_next_tick = 0;
    m_offset = 0;
}

// -----------------------------------------------------------------------------
void ScrollText::setText(int row, const char *text, Font *font)
{
    int16_t x = 0;
    int16_t y = row*16; 
    
    for( int j = 0 ; j < 16 ; j++ )
    {
        m_buffer[y+j].clear();
    }

    char *p = const_cast<char *>(text);
    uint16_t code;
    while( *p )
    {
        p = Font::getCharCodeAt(p, &code);
        Glyph *glyph = font->getGlyph(code);
        if( glyph )
        {
            for( int16_t i = 0 ; i < glyph->getWidth() ; i++ )
            {
                for( int16_t j = 0 ; j < 16 ; j++ )
                {
                    m_buffer[y+j].setPixel(x+i, (glyph->getData()[i] & (0x0001<<j))? 0x0F : 0x00);
                }
            }
            x += glyph->getAdvance();
        }
    }
    m_text_width[row] = x;
    Serial.print("scroll width [");
    Serial.print(row, DEC);
    Serial.print("] : ");
    Serial.println(m_text_width[row], DEC);
    if( m_text_width[row] > m_viewport_width )
    {
        // スクロール表示が必要な場合は、スクロール領域幅ぶんの「空白」を追加する
        x = m_text_width[row] + m_viewport_width;
        for( int j = 0 ; j < 16 ; j++ )
        {
            for( int i = 0 ; i < m_text_width[row] ; i++ )
            {
                m_buffer[y+j].setPixel(x+i, m_buffer[y+j].getPixel(i));
            }
        }
        m_text_width[row] += m_viewport_width;
    }
    else
    {
        m_text_width[row] = m_viewport_width;
    }
}

// -----------------------------------------------------------------------------
void ScrollText::startScroll()
{
    m_scroll_state = STATE_STATIC;
    m_offset = 0;
    m_next_tick = millis() + PAUSE_TIMEOUT;
}

// -----------------------------------------------------------------------------
int ScrollText::updateScroll()
{
    bool time_up = millis() >= m_next_tick;
    uint8_t updated = 0;
    switch( m_scroll_state )
    {
        case STATE_STATIC:
            if( time_up )
            {
                if( needScroll(0) )
                {
                    // 上段のスクロールを開始する
                    m_scroll_state = STATE_SCROLL_1;
                }
                else if( needScroll(1) )
                {
                    // 下段のスクロールを開始する
                    m_scroll_state = STATE_SCROLL_2;
                }
                else
                {
                    // 上段、下段とも、スクロールさせる必要はない
                    break;
                }
                m_offset = 0;
                m_next_tick = millis() + SCROLL_INTERVAL;
            }
            break;
        case STATE_SCROLL_1:
            if( time_up )
            {
                if( ++m_offset >= m_text_width[0] )
                {
                    // 上段のスクロールが終わった（表示は先頭に戻っている）
                    if( needScroll(1) )
                    {
                        // 下段のスクロールを開始する
                        m_offset = 0;
                        m_scroll_state = STATE_SCROLL_2;
                        m_next_tick += SCROLL_INTERVAL;
                    }
                    else
                    {
                        // 下段はスクロールの必要がないので、両段静止表示に戻る
                        m_scroll_state = STATE_STATIC;
                        m_next_tick = millis() + PAUSE_TIMEOUT;
                    }
                }
                else
                {
                    // 上段のスクロールを継続
                    m_next_tick += SCROLL_INTERVAL;
                }
                updated = 1;
            }
            break;
        case STATE_SCROLL_2:
            if( time_up )
            {
                if( ++m_offset >= m_text_width[1] )
                {
                    // 下段のスクロールが終わったので、両段静止表示に戻る
                    m_scroll_state = STATE_STATIC;
                    m_next_tick = millis() + PAUSE_TIMEOUT;
                }
                else
                {
                    // 下段のスクロールを継続
                    m_next_tick = millis() + SCROLL_INTERVAL;
                }
                updated = 2;
            }
            break;
    }
    return updated;
}

// -----------------------------------------------------------------------------
uint8_t *ScrollText::getLineData(int row, int16_t dy)
{
    if( isScrolling(row) )
    {
        return m_buffer[row*16+dy].getDataPtr(m_offset);
    }
    else
    {
        return m_buffer[row*16+dy].getDataPtr(0);
    }
}

// uint8_t ScrollText::getPixel(int row, int16_t dx, int16_t dy)
// {
//     if( isScrolling(row) )
//     {
//         int16_t x = (m_offset + dx) % m_text_width[row];
//         return (m_buffer[row][x] & (0x0001 << dy))? 0x0F : 0x00;
//     }
//     else
//     {
//         return (m_buffer[row][dx] & (0x0001 << dy))? 0x0F : 0x00;
//     }
// }

////////////////////////////////////////////////////////////////////////////////
//  SSD1322
////////////////////////////////////////////////////////////////////////////////
SSD1322::SSD1322(int csPin, int dcPin, int resetPin, int ePin, int rwPin)
{
    m_width    = 256;
    m_height   = 64;
    
    m_csPin    = csPin;
    m_dcPin    = dcPin;
    m_resetPin = resetPin;
    m_ePin     = ePin;
    m_rwPin    = rwPin;

    // Memory is addressed by row and column values. Each column address points to a 16-bit chunk,
    //  which corresponds to four 4-bit pixels. Here we assume that the column addresses that
    //  correspond to the displayed image are centred within each row.
    m_startCol = ((SSD1322_SEG_COUNT / 4) - (m_width / 4)) / 2;
    m_endCol   = ((SSD1322_SEG_COUNT / 4) - m_startCol) - 1;

    m_buf = NULL;

    m_popup_drawing = false;
}

// -----------------------------------------------------------------------------
void SSD1322::writeCmd(uint8_t c)
{
    // for( int p = 0 ; p < 8 ; p++ )
    // {
    //     digitalWrite(p+44, (c & (0x01 << p))? HIGH : LOW); 
    // }
    GPIO.P3 &= 0x00FF;
    GPIO.P3 |= (((uint16_t)c)<<8);
    digitalWrite(m_dcPin, LOW);
    digitalWrite(m_csPin, LOW);
    digitalWrite(m_rwPin, LOW);
    digitalWrite(m_ePin, HIGH);
    delayMicroseconds(1);
    digitalWrite(m_ePin, LOW);
    delayMicroseconds(1);
    digitalWrite(m_rwPin, HIGH);
    digitalWrite(m_csPin, HIGH);
    digitalWrite(m_dcPin, HIGH);
}

// -----------------------------------------------------------------------------
void SSD1322::writeData(uint8_t d)
{
    // for( int p = 0 ; p < 8 ; p++ )
    // {
    //     digitalWrite(p+44, (d & (0x01 << p))? HIGH : LOW); 
    // }
    GPIO.P3 &= 0x00FF;
    GPIO.P3 |= (((uint16_t)d)<<8);
    digitalWrite(m_dcPin, HIGH);
    digitalWrite(m_csPin, LOW);
    digitalWrite(m_rwPin, LOW);
    digitalWrite(m_ePin, HIGH);
    delayMicroseconds(1);
    digitalWrite(m_ePin, LOW);
    delayMicroseconds(1);
    digitalWrite(m_rwPin, HIGH);
    digitalWrite(m_csPin, HIGH);
    digitalWrite(m_dcPin, HIGH);
}

// -----------------------------------------------------------------------------
void SSD1322::reset(void)
{
    delay(1);  // Chip can require up to 1ms after power-on for VDD to stabalize
    digitalWrite(m_resetPin, LOW);
    delay(1);
    digitalWrite(m_resetPin, HIGH);
    delay(1);
}

// -----------------------------------------------------------------------------
bool SSD1322::initFrameBuffer(void)
{
    free(m_buf);

    // Calculate stride for each row
    int stridebits = m_width * SSD1322_DEPTH;
    m_stride = (stridebits / 8) + ((stridebits % 8) ? 1 : 0);

    // Allocate buffer
    m_buf = (uint8_t *)malloc(m_stride * m_height);
    if( !m_buf )
        return false;

    // Zero out buffer
    memset(m_buf, 0x00, m_stride * m_height);

    return true;
}

// -----------------------------------------------------------------------------
// Seems to set the number of display rows to be driven.
void SSD1322::setMuxRatio(int h)
{
    writeCmd(SSD1322_CMD_MUX_RATIO);
    writeData(h & 0x7F);
}

// -----------------------------------------------------------------------------
void SSD1322::setRowRange(int r1, int r2)
{
//  Serial.print("setRow ");
//  Serial.print(r1 & 0x7F);
//  Serial.print(", ");
//  Serial.print(r2 & 0x7F);
//  Serial.println();

    writeCmd(SSD1322_CMD_ROW_ADDRESS);
    writeData(r1 & 0x7F);  // Start
    writeData(r2 & 0x7F);  // End
}

// -----------------------------------------------------------------------------
void SSD1322::setColumnRange(int c1, int c2)
{
//  Serial.print("setColumn ");
//  Serial.print(c1 & 0x7F);
//  Serial.print(", ");
//  Serial.print(c2 & 0x7F);
//  Serial.println();

    writeCmd(SSD1322_CMD_COL_ADDRESS);
    writeData(c1 & 0x7F);  // Start
    writeData(c2 & 0x7F);  // End
}

// -----------------------------------------------------------------------------
void SSD1322::setClock(int freq, int divisor)
{
    writeCmd(SSD1322_CMD_CLOCK_DIVIDER);
    writeData(((freq & 0x0F) << 4) | (divisor & 0x0F));
}

// -----------------------------------------------------------------------------
void SSD1322::setRemap(uint8_t a, uint8_t b)
{
    writeCmd(SSD1322_CMD_REMAP);
    writeData(a);  // Bit 4 flips display vertically
    writeData(b);
}

// -----------------------------------------------------------------------------
bool SSD1322::init(void)
{
    pinMode(m_resetPin, OUTPUT);
    digitalWrite(m_resetPin, LOW);

    if (!initFrameBuffer())
        return false;
    
    pinMode(m_dcPin, OUTPUT);
    
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);

    pinMode(m_ePin, OUTPUT);
    digitalWrite(m_ePin, HIGH);

    pinMode(m_rwPin, OUTPUT);
    digitalWrite(m_rwPin, HIGH);

    for( int p = 44 ; p < 52 ; p++ )
    {
        pinMode(p, OUTPUT);
    }

    reset();

    // After power on, wait at least 300 ms before sending a command
    delay(300);

	writeCmd(SSD1322_CMD_COMLOCK);      // Send lock Command
	writeData(0x12);                        // Unlock device 
	
    writeCmd(SSD1322_CMD_DISPLAY_OFF);  // Turn the display off

    writeCmd(SSD1322_CMD_CLOCK_DIVIDER); // Set Front Clock Divider / Oscillator Frequency
    writeData(0x91);                        // 0b1001.0001

    writeCmd(SSD1322_CMD_MUX_RATIO);    // Set MUX Ratio
    writeData(0x3F);                        // Duty = 1/64

    writeCmd(SSD1322_CMD_OFFSET_LINE);  // Set Display Offset
    writeData(0x00);

    writeCmd(SSD1322_CMD_START_LINE);   // Set Display Start Line 
    writeData(0x00);

    writeCmd(SSD1322_CMD_COL_ADDRESS);  // Set Column Address
    writeData(0x1C);                        // Start column (offset of 28)
    writeData(0x5B);                        // End column = 256, 4 per column = 64-1+offset(28) = 91
    
    writeCmd(SSD1322_CMD_ROW_ADDRESS);  // Set Row Address
    writeData(0x00);                        // Start row
    writeData(0x3F);                        // 64-1 rows

    writeCmd(SSD1322_CMD_REMAP);        // Set Re-map and Dual COM Line mode
    writeData(0x14);                        // Horizontal address increment, Disable Column Address Re-map, Enable Nibble Re-map, Scan from COM0 to COM[N1] (where N is the Multiplex ratio), Disable COM Split Odd Even
    writeData(0x11);                        // Enable Dual COM mode (MUX = 63)
    
    writeCmd(SSD1322_CMD_VDDSEL);	    // Function Selection
    writeData(0x01);                        // Enable internal VDD  regulator

    writeCmd(SSD1322_CMD_DISPENHA);     // Display Enhancement A
    writeData(0xA0);
    writeData(0xfd);

    writeCmd(SSD1322_CMD_CONTRSTCUR);   // Set Contrast Current
    writeData(0x9f);

    writeCmd(SSD1322_CMD_MSTCONTRST);   // Master Contrast Current Control
    writeData(0x0f);

    // writeCmd(SSD1322_CMD_DEFGRYTABLE);  // Select Default Linear Gray Scale table
    setGrayscaleTable();

    writeCmd(SSD1322_CMD_ENGREYSCALE);  // Enable Gray Scale table 

    writeCmd(SSD1322_CMD_PHASELEN);     // Set Phase Length
    writeData(0xE2);

    writeCmd(SSD1322_CMD_DISPENHB);     // Display Enhancement  B
    writeData(0x82);	                    // User is recommended to set A[5:4] to 00b.
    writeData(0x20);                        // Default

    writeCmd(SSD1322_CMD_PRECHRGVOL);   // Set Pre-charge voltage
    writeData(0x1F);

    writeCmd(SSD1322_CMD_SECPRECHRG);   // Set Second Precharge Period
    writeData(0x08);

    writeCmd(SSD1322_CMD_SETVCOMH);     // Set VCOMH
    writeData(0x07);    

    writeCmd(SSD1322_CMD_MODE_NORMAL);  // Normal Display

    writeCmd(SSD1322_CMD_DISPLAY_ON);   // Sleep mode OFF

    m_font[FONT_TINY ].load("font_t.dat", 9);
    m_font[FONT_SMALL].load("font_s.dat", 14);
    m_font[FONT_LARGE].load("font_l.dat", 16);

    return true;
}

// -----------------------------------------------------------------------------
void SSD1322::displayOn(void)
{
    writeCmd(SSD1322_CMD_DISPLAY_ON);
    delay(200);  // Wait for display to turn on
}

// -----------------------------------------------------------------------------
void SSD1322::displayOff(void)
{
    writeCmd(SSD1322_CMD_DISPLAY_OFF);
}

// -----------------------------------------------------------------------------
void SSD1322::setDisplayMode(SSD1322_Mode mode)
{
    switch (mode)
    {
        case SSD1322_MODE_ALL_ON:
            writeCmd(SSD1322_CMD_MODE_ALL_ON);
            break;
        case SSD1322_MODE_ALL_OFF:
            writeCmd(SSD1322_CMD_MODE_ALL_OFF);
            break;
        case SSD1322_MODE_INVERT:
            writeCmd(SSD1322_CMD_MODE_INVERT);
            break;
        default:
            writeCmd(SSD1322_CMD_MODE_NORMAL);
            break;
    }
}

// -----------------------------------------------------------------------------
void SSD1322::setStartRow(int r)
{
    writeCmd(SSD1322_CMD_START_LINE);
    writeData(r & 0x7F);
}

// -----------------------------------------------------------------------------
void SSD1322::setOffsetRow(int r)
{
    writeCmd(SSD1322_CMD_OFFSET_LINE);
    writeData(r & 0x7F);
}

// -----------------------------------------------------------------------------
void SSD1322::setGrayscaleTable()
{
    uint8_t lut[15] = {0, 2, 5, 9, 14, 21, 29, 38, 48, 60, 73, 87, 102, 118, 136};
    writeCmd(SSD1322_CMD_SETGRAYTABLE);
    for( int n = 0 ; n < 15 ; n++ )
    {
        writeData(lut[n]);
    }
}

// -----------------------------------------------------------------------------
uint8_t SSD1322::getPixel(int16_t x, int16_t y)
{
    uint32_t addr = (y * m_stride) + (x / 2);
    uint8_t data = m_buf[addr];
    if (x % 2)
        return (data & 0x0F);
    else
        return (data & 0xF0) >> 4;
}

// -----------------------------------------------------------------------------
void SSD1322::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if( !m_buf )
    {
        return;
    }

    if( (x < 0) || (x >= m_width) || (y < 0) || (y >= m_height) )
    {
        return;
    }

    if( !m_popup_rect.isEmpty() )
    {
        // ポップアップが表示中で、かつ、描画対象ピクセルがポップアップ矩形領域内の場合
        // m_popup_drawing == true でないと、描画はできない
        if( m_popup_rect.include(x, y) && !m_popup_drawing )
        {
            return;
        }
    }

    uint32_t addr = (y * m_stride) + (x / 2);
    uint8_t data = m_buf[addr];
    if (x % 2)
        data = (data & 0xF0) | (color & 0x0F);
    else
        data = (data & 0x0F) | ((color & 0x0F) << 4);
    m_buf[addr] = data;
}

// -----------------------------------------------------------------------------
void SSD1322::drawHzLine(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    for( int16_t e = x + len ; x < e ; x++ )
    {
        drawPixel(x, y, color);
    }
}

// -----------------------------------------------------------------------------
void SSD1322::drawVtLine(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    for( int16_t e = y + len ; y < e ; y++ )
    {
        drawPixel(x, y, color);
    }
}

// -----------------------------------------------------------------------------
void SSD1322::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    drawHzLine(x, y, w, color);
    drawVtLine(x, y, h, color);
    drawHzLine(x, y+h-1, w, color);
    drawVtLine(x+w-1, y, h, color);
}
void SSD1322::drawRect(Rectangle& rc, uint16_t color)
{
    drawRect(rc.left, rc.top, rc.width, rc.height, color);
}

// -----------------------------------------------------------------------------
void SSD1322::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    for( int16_t e = y + h ; y < e ; y++ )
    {
        drawHzLine(x, y, w, color);
    }
}
void SSD1322::fillRect(Rectangle& rc, uint16_t color)
{
    fillRect(rc.left, rc.top, rc.width, rc.height, color);
}

// -----------------------------------------------------------------------------
int16_t SSD1322::drawChar(int16_t x, int16_t y, uint16_t code, uint8_t size, uint8_t color)
{
    Glyph *glyph = m_font[size].getGlyph(code);
    if( !glyph )
    {
        return x;
    }
    int16_t width = glyph->getWidth();
    int16_t height = glyph->getHeight();
    int16_t left = x + glyph->getOffset();
    if( left < 0 )
    {
        left = 0;
    }
    for( int16_t j = 0 ; j < height ; j++ )
    {
        for( int16_t i = 0 ; i < width ; i++ )
        {
            uint8_t p = glyph->getPixel(i, j);
            if( p )
            {
                drawPixel(left+i, y+j, color);
            }
        }
    }
    return x + glyph->getAdvance();
}

// -----------------------------------------------------------------------------
int16_t SSD1322::drawASCIIText(int16_t x, int16_t y, const char *text, uint8_t size, uint8_t color)
{
    for( int i = 0 ; text[i] ; i++ )
    {
        x = drawChar(x, y, text[i], size, color);
    }
    return x;
}

// -----------------------------------------------------------------------------
int16_t SSD1322::drawString(int16_t x, int16_t y, const char *text, uint8_t size, uint8_t color)
{
    char *p = const_cast<char *>(text);
    uint16_t code;
    while( *p )
    {
        p = Font::getCharCodeAt(p, &code);
        x = drawChar(x, y, code, size, color);
    }
    return x;
}

// -----------------------------------------------------------------------------
int16_t SSD1322::getTextWidth(const char *text, uint8_t size)
{
    char *p = const_cast<char *>(text);
    uint16_t code;
    int16_t w = 0;
    while( *p )
    {
        p = Font::getCharCodeAt(p, &code);
        Glyph *glyph = m_font[size].getGlyph(code);
        if( glyph )
        {
            w += glyph->getAdvance();
        }
    }
    return w;
}

// -----------------------------------------------------------------------------
void SSD1322::drawImage(int16_t x, int16_t y, Image *image)
{
    int16_t w = image->getWidth();
    int16_t h = image->getHeight();
    for( int j = 0 ; j < h ; j++ )
    {
        for( int i = 0 ; i < w ; i++ )
        {
            drawPixel(x+i, y+j, image->getPixel(i, j));
        }
    }
}

// -----------------------------------------------------------------------------
bool SSD1322::getPopupRectAddress(int16_t y, uint32_t& start, uint32_t& end)
{
    if( m_popup_rect.isEmpty() || y < m_popup_rect.top || m_popup_rect.bottom() < y )
    {
        return false;
    }
    start = y*m_stride + m_popup_rect.left / 2;
    end   = start + m_popup_rect.width / 2;
    return true;
}

// ---------------------------------------------------------------------------
void SSD1322::drawScrollText(int row, int16_t x, int16_t y, ScrollText *text)
{
    int16_t width = text->getViewportWidth() / 2;
    uint32_t s, e;
    for( int16_t j = 0 ; j < ScrollText::VIEWPORT_HEIGHT ; j++ )
    {
        uint32_t addr = (y+j)*m_stride + x/2;
        uint8_t *data = text->getLineData(row, j);
        if( getPopupRectAddress(y+j, s, e) )
        {
            for( int16_t i = 0 ; i < width ; i++, addr++ )
            {
                if( addr < s || e < addr )
                {
                    m_buf[addr] = data[i];
                }
            }
        }
        else
        {
            memcpy(m_buf+addr, data, width);
        }
    }
}

//------------------------------------------------------------------------------
void SSD1322::clear(uint8_t color)
{
    uint8_t b = ((color & 0xF) << 4) | (color & 0xF);
    memset(m_buf, b, m_stride * m_height);
}

//------------------------------------------------------------------------------
void SSD1322::display(void)
{
    setColumnRange(m_startCol, m_endCol);
    setRowRange(0, m_height - 1);
        
    writeCmd(SSD1322_CMD_WRITE_RAM);

    for (int y = 0; y < m_height; y++)
    {
        uint32_t addr = y * m_stride;
        for (int s = 0; s < m_stride; s++)
        {
            writeData(m_buf[addr++]);
        }
    }  
}

//------------------------------------------------------------------------------
void SSD1322::invalidateRect(int16_t left, int16_t top, int16_t w, int16_t h)
{
    int col_l = m_startCol + left/4;        // 領域の左端（を含む）カラムのアドレス
    int col_r = m_startCol + (left + w)/4;  // 領域の右端（を含む）カラムのアドレス
    if( col_r > m_endCol )
    {
        col_r = m_endCol;
    }
    setColumnRange(col_l, col_r);
    
    int16_t bottom = top + h;
    setRowRange(top, bottom-1);

    writeCmd(SSD1322_CMD_WRITE_RAM);
    int start_byte = (left/4)*2;                // 水平方向のアドレス（バイト単位）のオフセット
    int byte_len   = (col_r - col_l + 1) * 2;   // 水平方向のバイト長
    for( int y = top ; y < bottom ; y++ )
    {
        uint32_t addr = y * m_stride + start_byte;
        for( int b = 0 ; b < byte_len ; b++, addr++ )
        {
            writeData(m_buf[addr]);
        }
    }
}
void SSD1322::invalidateRect(Rectangle& rc)
{
    invalidateRect(rc.left, rc.top, rc.width, rc.height);
}

// -----------------------------------------------------------------------------
void SSD1322::enablePopup(Rectangle& rc)
{
    m_popup_rect = rc;
    m_popup_drawing = false;
}

// -----------------------------------------------------------------------------
void SSD1322::disablePopup()
{
    m_popup_rect.empty();
    m_popup_drawing = false;
}

// -----------------------------------------------------------------------------
void SSD1322::startDrawPopup()
{
    if( !m_popup_rect.isEmpty() )
    {
        m_popup_drawing = true;
    }
}

// -----------------------------------------------------------------------------
void SSD1322::endDrawPopup()
{
    m_popup_drawing = false;
}
