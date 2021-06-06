#include <Arduino.h>
#include "oled_display.h"
#include "M41T62.h"
#include "eeprom_24lc.h"

////////////////////////////////////////////////////////////////////////////////
//  View
////////////////////////////////////////////////////////////////////////////////
Vector<View *> View::m_views; 

View::View(SSD1322 *oled, Playlist *playlist, uint8_t id)
    : m_visible(false), m_oled(oled), m_playlist(playlist), m_id(id)
{
    if( m_views.capacity() == 0 )
    {
        m_views.alloc(NUM_VIEWS);
    }
    m_views.push_back(this);
}

// -----------------------------------------------------------------------------
void View::init()
{

}

// -----------------------------------------------------------------------------
View *View::getView(uint8_t id)
{
    for( uint16_t i = 0 ; i < m_views.size() ; i++ )
    {
        if( m_views[i]->m_id == id )
        {
            return m_views[i];
        }
    }
    return NULL;
}

// -----------------------------------------------------------------------------
View *View::getActiveView()
{
    for( uint16_t i = 0 ; i < m_views.size() ; i++ )
    {
        if( m_views[i]->m_visible )
        {
            return m_views[i];
        }
    }
    return NULL;
}

// -----------------------------------------------------------------------------
void View::show(uint8_t id)
{
    for( uint16_t i = 0 ; i < m_views.size() ; i++ )
    {
        if( m_views[i]->m_id == id )
        {
            m_views[i]->show();
        }        
        else
        {
            m_views[i]->hide();
        }
    }
}

// -----------------------------------------------------------------------------
void View::show()
{
    m_visible = true;
    m_oled->clear(0x00);
    refresh();
    m_oled->display();
}

// -----------------------------------------------------------------------------
void View::hide()
{
    m_visible = false;
}

// -----------------------------------------------------------------------------
void View::refresh()
{

}

// -----------------------------------------------------------------------------
void View::update()
{

}

// -----------------------------------------------------------------------------
void View::invalidate(bool partial)
{
    if( m_visible )
    {
        m_oled->clear(0x00);
        refresh();
        m_oled->display();
    }
}

////////////////////////////////////////////////////////////////////////////////
//  BlankView
////////////////////////////////////////////////////////////////////////////////
// BlankView::BlankView(SSD1322 *oled, Playlist *playlist)
//     : View(oled, playlist, BlankView::ID)
// {

// }

////////////////////////////////////////////////////////////////////////////////
//  PlaybackView
////////////////////////////////////////////////////////////////////////////////
PlaybackView::PlaybackView(SSD1322 *oled, Playlist *playlist, SpectrumAnalyzer *analyzer)
    : View(oled, playlist, PlaybackView::ID), m_state(STOP), m_analyzer(analyzer),
    m_spectrum_gain(2)
{
    for( int n = 0 ; n < 2 ; n++ )
    {
        m_track_number[n] = 0;
    }
    for( int n = 0 ; n < 5 ; n++ )
    {
        m_elapsed_time[n] = 0;
    }
    for( int n = 0 ; n < SpectrumAnalyzer::NUM_CHANNELS ; n++ )
    {
        m_spectrum[n] = 0;
    }
}

// -----------------------------------------------------------------------------
void PlaybackView::init()
{
    View::init();
    m_digit_images.load("digits.dat", 11);
    m_state_images.load("playback.dat", 3);
    m_label_images.load("label_1.dat", 2);
    m_icon_images.load("icon.dat", 3);

    m_scroll_text.init(236);
}

// -----------------------------------------------------------------------------
void PlaybackView::update()
{
    drawState(false);
    drawTrackNumber(false);
    drawElapsedTime(false);
    drawSpectrum(false);
    int r = m_scroll_text.updateScroll();
    if( r )
    {
        drawTrackInfo(r-1);
    }
}

// -----------------------------------------------------------------------------
void PlaybackView::refresh()
{
    View::refresh();

    m_oled->drawASCIIText(24, 1, "TRACK", SSD1322::FONT_TINY, 0x0A);
    m_oled->drawASCIIText(98, 1, "TIME", SSD1322::FONT_TINY, 0x0A);
    // m_oled->drawImage(24, 2, m_label_images.getImage(1));
    // m_oled->drawImage(100, 2, m_label_images.getImage(0));
    drawState(true);
    drawTrackNumber(true);
    drawElapsedTime(true);
    drawSpectrum(true);
    drawAlbumInfo(true);
    setScrollText();
    drawTrackInfo();
}

// -----------------------------------------------------------------------------
void PlaybackView::invalidate(bool partial)
{
    if( partial )
    {
        drawTrackInfo(1);
    }
    else
    {
        View::invalidate(false);
    }
}

// -----------------------------------------------------------------------------
void PlaybackView::setScrollText()
{
    char text[256];
    Artist *artist = m_playlist->getSelectedArtist();
    Album *album = artist->getSelectedAlbum();
    if( Player().isStopped() )
    {
        // 停止中は、上段にアーティスト名、下段にアルバムタイトルを表示
        m_scroll_text.setText(0, artist->getName(), m_oled->getFont(SSD1322::FONT_LARGE));
        m_scroll_text.setText(1, album->getTitle(), m_oled->getFont(SSD1322::FONT_LARGE));
    }
    else
    {
        // 再生中は、上段に「曲名」を表示、
        // 下段は「下段にアルバムタイトル」＋「アーティスト名」を表示
        m_scroll_text.setText(0, album->getCurrentSong()->getTitle(), m_oled->getFont(SSD1322::FONT_LARGE));
        strcpy(text, album->getTitle());
        strcat(text, " - ");
        strcat(text, artist->getName());
        m_scroll_text.setText(1, text, m_oled->getFont(SSD1322::FONT_LARGE));
    }
    m_scroll_text.startScroll();
}

// -----------------------------------------------------------------------------
void PlaybackView::drawState(bool force_redraw)
{
    uint8_t state = Player().isStopped()? STOP : (Player().isPaused()? PAUSE : PLAY);
    if( (m_state != state) || force_redraw )
    {
        m_oled->drawImage(0, 0, m_state_images.getImage(state));
        if( !force_redraw )
        {
            m_oled->invalidateRect(0, 0, 22, 22);
        }
    }
    m_state = state;
}

// -----------------------------------------------------------------------------
void PlaybackView::drawTrackNumber(bool force_redraw)
{
    uint8_t tracknum = 0;
    if( !Player().isStopped() )
    {
        tracknum = m_playlist->getSelectedArtist()->getSelectedAlbum()->getCurrentSong()->getTrackIndex();
    }
    uint8_t num[2];
    num[0] = tracknum / 10;
    num[1] = tracknum % 10;
    bool track_changed = false;
    for( int n = 0 ; n < 2 ; n++ )
    {
        if( num[n] != m_track_number[n] || force_redraw )
        {
            m_oled->drawImage(56+12*n, 0, m_digit_images.getImage(num[n]));
            if( !force_redraw )
            {
                m_oled->invalidateRect(56+12*n, 0, 12, 22);
            }
            m_track_number[n] = num[n];
            track_changed = true;
        }
    }
    if( !force_redraw && track_changed )
    {
        drawTrackInfo(false);
    }
}

// -----------------------------------------------------------------------------
void PlaybackView::drawElapsedTime(bool force_redraw)
{
    uint32_t t = 0;
    if( !Player().isStopped() )
    {
        t = Player().getElapsed();
    }

    uint8_t num[5];
    num[0] = (uint8_t)((t / 60) / 10);
    num[1] = (uint8_t)((t / 60) % 10);
    num[2] = 10;
    num[3] = (uint8_t)((t % 60) / 10);
    num[4] = (uint8_t)((t % 60) % 10);
    for( int n = 0 ; n < 5 ; n++ )
    {
        if( num[n] != m_elapsed_time[n] || force_redraw )
        {
            m_oled->drawImage(125+12*n, 0, m_digit_images.getImage(num[n]));
            if( !force_redraw )
            {
                m_oled->invalidateRect(125+12*n, 0, 12, 22);
            }
            m_elapsed_time[n] = num[n];
        }
    }
}

// -----------------------------------------------------------------------------
void PlaybackView::drawSpectrum(bool force_redraw)
{
    int16_t spc[SpectrumAnalyzer::NUM_CHANNELS], 
            inv[SpectrumAnalyzer::NUM_CHANNELS];
    bool modified = false;

    static uint32_t tick = 0;

    if( Player().isStopped() )
    {
        m_analyzer->disable();
        return;
    }

    if( !force_redraw && (millis() <= tick) )
    {
        return;
    }
    if( Player().isStopped() || Player().isPaused() )
    {
        m_analyzer->disable();
    }
    else
    {
        m_analyzer->enable();
        m_analyzer->update();
    }
    for( int ch = 0 ; ch < SpectrumAnalyzer::NUM_CHANNELS ; ch++ )        
    {
        spc[ch] = m_analyzer->getValue(ch) / m_spectrum_gain;
        if( spc[ch] > 22 )
        { 
            spc[ch] = 22; 
        }
        inv[ch] = 22 - spc[ch];
        if( m_spectrum[ch] != spc[ch] )
        {
            modified = true;
        }
    }
    if( force_redraw || modified )
    {
        for( int ch = 0 ; ch < SpectrumAnalyzer::NUM_CHANNELS ; ch++ )
        {
            int16_t x = 207 + ch*7;
            if( inv[ch] )
            {
                m_oled->fillRect(x, 0, 6, inv[ch], 0x05);
            }
            if( spc[ch] )
            {
                m_oled->fillRect(x, inv[ch], 6, spc[ch], 0x0F);
            }
        }
        if( !force_redraw )
        {
            m_oled->invalidateRect(207, 0, 49, 22);
        }
    }
    for( int ch = 0 ; ch < SpectrumAnalyzer::NUM_CHANNELS ; ch++ )
    {
        m_spectrum[ch] = spc[ch];
    }
    tick = millis() + 10;
}

// -----------------------------------------------------------------------------
void PlaybackView::drawAlbumInfo(bool force_redraw)
{
    if( Player().isStopped() )
    {
        Album *album = m_playlist->getSelectedArtist()->getSelectedAlbum();
        uint16_t num = album->getSongCount();
        char str[16];
        str[1] = '0'+(num % 10);
        num /= 10;
        str[0] = num? ('0'+num) : ' ';
        strcpy(str+2, " TRACKS");
        int16_t w = m_oled->getTextWidth(str, SSD1322::FONT_TINY);
        m_oled->drawASCIIText(256-w, 1, str, SSD1322::FONT_TINY, 0x0A);

        uint16_t total = album->getTotalLength();
        str[0] = (total / 600)+'0';
        str[1] = ((total / 60) % 10)+'0';
        str[2] = ':';
        str[3] = ((total % 60) / 10)+'0';
        str[4] = ((total % 60) % 10)+'0';
        str[5] = 0x00;
        w = m_oled->getTextWidth(str, SSD1322::FONT_TINY);
        m_oled->drawASCIIText(256-w, 12, str, SSD1322::FONT_TINY, 0x0A);
    }
}

// -----------------------------------------------------------------------------
void PlaybackView::drawTrackInfo(int row)
{
    if( row < 0 )
    {
        if( Player().isStopped() )
        {
            m_oled->drawImage(0, 28, m_icon_images.getImage(0));
            m_oled->drawImage(0, 48, m_icon_images.getImage(1));
        }
        else
        {
            m_oled->drawImage(0, 28, m_icon_images.getImage(2));
            m_oled->drawImage(0, 48, m_icon_images.getImage(1));
        }
    }

    for( int n = 0 ; n < 2 ; n++ )
    {
        if( n == row || row < 0 )
        {
            m_oled->drawScrollText(n, 20, 28+20*n, &m_scroll_text);
            if( row >= 0 )
            {
                m_oled->invalidateRect(20, 28+20*n, 236, 16);
                // for( int j = 0 ; j < 4 ; j += 2)
                // {
                //     m_oled->invalidateRect(20, 28+20*n+j, 236, 2);
                //     m_oled->invalidateRect(20, 36+20*n+j, 236, 2);
                // }
            }
        }
    }

    // m_oled->fillRect(0, 28, 256, 36, 0x00);
    // if( Player().isStopped() )
    // {
    //     // 停止中は、上段にアーティスト名、下段にアルバムタイトルを表示
    //     const char *str = m_playlist->getSelectedArtist()->getName();
    //     m_oled->drawString(0, 28, str, SSD1322::FONT_LARGE, 0x0F);
    //     str = m_playlist->getSelectedArtist()->getSelectedAlbum()->getTitle();
    //     m_oled->drawString(0, 48, str, SSD1322::FONT_LARGE, 0x0F);
    // }
    // else
    // {
    //     // 再生中は、上段に「曲名」、下段に「下段にアルバムタイトル - アーティスト名」を表示
    //     const char *str = m_playlist->getSelectedArtist()->getSelectedAlbum()->getCurrentSong()->getTitle();
    //     m_oled->drawString(0, 28, str, SSD1322::FONT_LARGE, 0x0F);
    //     str = m_playlist->getSelectedArtist()->getSelectedAlbum()->getTitle();
    //     int16_t x = m_oled->drawString(0, 48, str, SSD1322::FONT_LARGE, 0x0F);
    //     x = m_oled->drawString(x, 48, " - ", SSD1322::FONT_LARGE, 0x0F);
    //     str = m_playlist->getSelectedArtist()->getName();
    //     m_oled->drawString(x, 48, str, SSD1322::FONT_LARGE, 0x0F);
    // }
    // if( !force_redraw )
    // {
    //     m_oled->invalidateRect(0, 28, 256, 36);
    // }
}

// -----------------------------------------------------------------------------
bool PlaybackView::handleIRR(IRRCODE code)
{
    if( code == IRRemote::ST_REPT )
    {
        View::show(ConfigView::ID);
        return true;
    }
    if( code == IRRemote::UP || code == IRRemote::DOWN )
    {
        // 現在のアーティストのアルバム選択画面に切り替える
        AlbumListView *view = (AlbumListView *)View::getView(AlbumListView::ID);
        view->setArtist(m_playlist->getSelectedArtist());
        View::show(AlbumListView::ID);
        return true;
    }
    if( code == IRRemote::POWER )
    {
        if( --m_spectrum_gain < 1 )
        {
            m_spectrum_gain = 4;
        }
        return true;
    }

    int d = IRRemote::codeToDigit(code);
    if( d <= 0 )
    {
        return false;
    }
    Album *album = m_playlist->getSelectedArtist()->getSelectedAlbum();
    if( d <= album->getSongCount() )
    {
        if( !Player().isStopped() )
        {
            Player().stop(true);
        }
        album->seekTo((uint16_t)(d-1));
        const char *filename = album->getCurrentSong()->getFileName();
        Serial.println(filename);
        Player().play(filename);
        show();
    }
    return true;

    // // if( code == IRRemote::NINE )
    // // {
    // // }

    // return false;
}




////////////////////////////////////////////////////////////////////////////////
//  ListView
////////////////////////////////////////////////////////////////////////////////
ImageList ListView::m_cursor_images;

// -----------------------------------------------------------------------------
ListView::ListView(SSD1322 *oled, Playlist *playlist, uint8_t id)
    : View(oled, playlist, id), m_item_count(0), m_page_count(0), 
    m_page_pos(0), m_cursor_pos(0)
{

}

// -----------------------------------------------------------------------------
void ListView::init()
{
    View::init();
    if( m_cursor_images.getCount() == 0 )
    {
        m_cursor_images.load("cursor.dat", 1);
    }
}

// -----------------------------------------------------------------------------
void ListView::show()
{
    m_item_count = getItemCount();
    m_page_count = (m_item_count / ITEMS_PER_PAGE) + ((m_item_count % ITEMS_PER_PAGE)? 1 : 0);
    uint16_t index = getInitialSelection();
    m_page_pos = index / ITEMS_PER_PAGE;
    m_cursor_pos = index % ITEMS_PER_PAGE;

    View::show();
}

// -----------------------------------------------------------------------------
void ListView::refresh()
{
    View::refresh();

    drawHeader();
    drawIndex();
    drawListItems();
}

// -----------------------------------------------------------------------------
void ListView::drawIndex()
{
    char str[10];
    uint16_t i = 1+cursorPosToIndex(m_cursor_pos);
    str[0] = (i / 10)? ('0'+ i/10) : ' ';
    str[1] = '0' + (i % 10);
    str[2] = '/';
    uint16_t n = m_item_count;
    str[3] = (n / 10)? ('0'+ n/10) : ' ';
    str[4] = '0' + (n % 10);
    str[5] = '\0';
    m_oled->fillRect(221, 0, 35, 15, 0x00);
    m_oled->drawString(221, 0, str, SSD1322::FONT_SMALL, 0x0F);
}

// -----------------------------------------------------------------------------
void ListView::drawListItems()
{
    for( uint16_t n = 0 ; n < ITEMS_PER_PAGE ; n++ )
    {
        Rectangle rc;
        getItemRect(n, rc);
        uint16_t index = cursorPosToIndex(n);
        bool selected = (n == m_cursor_pos)? true : false;
        if( index < m_item_count )
        {
            drawItem(cursorPosToIndex(n), rc, selected);
        }
        else
        {
            m_oled->fillRect(rc, 0x00);
        }
    }
}

// -----------------------------------------------------------------------------
void ListView::drawItem(uint16_t index, Rectangle& rc, bool selected)
{
    m_oled->fillRect(rc.left, rc.top, 16, 16, 0x00);
    m_oled->fillRect(rc.left+16, rc.top, rc.width-16, rc.height, selected? 0x02 : 0x00);
    if( selected )
    {
        m_oled->drawImage(rc.left, rc.top, m_cursor_images.getImage(0));
    }
}

// -----------------------------------------------------------------------------
void ListView::moveCursor(int direction)
{
    Rectangle rc;

    if( direction < 0 )
    {
        if( m_cursor_pos > 0 )
        {
            getItemRect(m_cursor_pos, rc);
            drawItem(cursorPosToIndex(m_cursor_pos), rc, false);
            m_oled->invalidateRect(rc);
            --m_cursor_pos;
            getItemRect(m_cursor_pos, rc);
            drawItem(cursorPosToIndex(m_cursor_pos), rc, true);
            m_oled->invalidateRect(rc);
            drawIndex();
            m_oled->invalidateRect(221, 0, 35, 16);
        }
        else if( m_page_pos > 0 )
        {
            --m_page_pos;
            m_cursor_pos = ITEMS_PER_PAGE - 1;
            drawListItems();
            m_oled->invalidateRect(0, 16, 256, 48);
            drawIndex();
            m_oled->invalidateRect(221, 0, 35, 16);
        }
    }
    else
    {
        if( m_cursor_pos < (ITEMS_PER_PAGE-1) ) // cursorPosToIndex(m_cursor_pos) < (m_item_count - 1) )
        {
            getItemRect(m_cursor_pos, rc);
            drawItem(cursorPosToIndex(m_cursor_pos), rc, false);
            m_oled->invalidateRect(rc);
            ++m_cursor_pos;
            getItemRect(m_cursor_pos, rc);
            drawItem(cursorPosToIndex(m_cursor_pos), rc, true);
            m_oled->invalidateRect(rc);
            drawIndex();
            m_oled->invalidateRect(221, 0, 35, 16);
        }
        else if( m_page_pos < (m_page_count - 1) )
        {
            ++m_page_pos;
            m_cursor_pos = 0;
            drawListItems();
            m_oled->invalidateRect(0, 16, 256, 48);
            drawIndex();
            m_oled->invalidateRect(221, 0, 35, 16);
        }
    }
}

// -----------------------------------------------------------------------------
void ListView::movePage(int direction)
{
    if( direction < 0 )
    {
        if( m_page_pos > 0 )
        {
            --m_page_pos;
            m_cursor_pos = ITEMS_PER_PAGE - 1;
            drawListItems();
            m_oled->invalidateRect(0, 16, 256, 48);
            drawIndex();
            m_oled->invalidateRect(221, 0, 35, 16);
        }
    }
    else
    {
        if( m_page_pos < (m_page_count - 1) )
        {
            ++m_page_pos;
            m_cursor_pos = 0;
            drawListItems();
            m_oled->invalidateRect(0, 16, 256, 48);
            drawIndex();
            m_oled->invalidateRect(221, 0, 35, 16);
        }
    }
}

// -----------------------------------------------------------------------------
bool ListView::handleIRR(IRRCODE code)
{
    switch( code )
    {
        case IRRemote::UP:
            moveCursor(-1);
            break;
        case IRRemote::DOWN:
            moveCursor(1);
            break;
        case IRRemote::TWO:
            movePage(-1);
            break;
        case IRRemote::EIGHT:
            movePage(1);
            break;
        default:
            return false;
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//  AlbumListView
////////////////////////////////////////////////////////////////////////////////
AlbumListView::AlbumListView(SSD1322 *oled, Playlist *playlist)
    : ListView(oled, playlist, AlbumListView::ID), m_artist(NULL)
{

}

// -----------------------------------------------------------------------------
uint16_t AlbumListView::getItemCount()
{
    if( m_artist )
    {
        return m_artist->getAlbumCount();
    }
    return 0;
}

// -----------------------------------------------------------------------------
uint16_t AlbumListView::getInitialSelection()
{
    if( m_artist )
    {
        Album *album = m_artist->getSelectedAlbum();
        return m_artist->getIndexOfAlbum(album);
    }
    return 0;
}

// -----------------------------------------------------------------------------
void AlbumListView::drawHeader()
{
    int16_t x = m_oled->drawString(0, 0, "「", SSD1322::FONT_SMALL, 0x0F);
    x = m_oled->drawString(x, 0, m_artist->getName(), SSD1322::FONT_SMALL, 0x0F);
    m_oled->drawString(x, 0, "」のアルバム", SSD1322::FONT_SMALL, 0x0F);
    m_oled->drawHzLine(0, 15, 256, 0x0F);
}

// -----------------------------------------------------------------------------
void AlbumListView::drawItem(uint16_t index, Rectangle& rc, bool selected)
{
    ListView::drawItem(index, rc, selected);
    Album *album = m_artist->getAlbums()[index];
    m_oled->drawString(rc.left+18, rc.top, album->getTitle(), SSD1322::FONT_SMALL, selected? 0x0F : 0x07);
}

// -----------------------------------------------------------------------------
Album *AlbumListView::getSelection()
{
    return m_artist->getAlbums()[getSelectedIndex()];
}

// -----------------------------------------------------------------------------
bool AlbumListView::handleIRR(IRRCODE code)
{
    if( ListView::handleIRR(code) )
    {
        return true;
    }
    switch( code )
    {
        case IRRemote::ST_REPT:
            // [ST/REPT]キー : アルバム(アーティスト)を選択し、プレイバック画面に切り替える
            Player().stop(true);
            m_playlist->selectArtistByID(m_artist->getID());
            m_artist->selectAlbumByID(getSelection()->getID());
            m_playlist->save();
            // breakしない
        case IRRemote::ZERO:
            // [0]キー : プレイバック画面に切り替える
            View::show(PlaybackView::ID);
            break;
        case IRRemote::NINE:
            // [9]キー: アーティスト選択画面に切り替える
            {
                ArtistListView *view = (ArtistListView *)(View::getView(ArtistListView::ID));
                view->setArtist(m_artist);
                View::show(ArtistListView::ID);
            }
            break;
        default:
            return false;
    }
    return true;
}



////////////////////////////////////////////////////////////////////////////////
//  ArtistListView
////////////////////////////////////////////////////////////////////////////////
ArtistListView::ArtistListView(SSD1322 *oled, Playlist *playlist)
    : ListView(oled, playlist, ArtistListView::ID), m_artist(NULL)
{
}

// -----------------------------------------------------------------------------
uint16_t ArtistListView::getItemCount()
{
    return m_playlist->getArtistCount();
}

// -----------------------------------------------------------------------------
uint16_t ArtistListView::getInitialSelection()
{
    if( m_artist )
    {
        return m_playlist->getIndexOfArtist(m_artist);
    }
    return 0;
}

// -----------------------------------------------------------------------------
void ArtistListView::drawHeader()
{
    m_oled->drawString(0, 0, "アーティスト選択", SSD1322::FONT_SMALL, 0x0F);
    m_oled->drawHzLine(0, 15, 256, 0x0F);
}

// -----------------------------------------------------------------------------
void ArtistListView::drawItem(uint16_t index, Rectangle& rc, bool selected)
{
    ListView::drawItem(index, rc, selected);
    Artist *artist = m_playlist->getArtists()[index];
    m_oled->drawString(rc.left+18, rc.top, artist->getName(), SSD1322::FONT_SMALL, selected? 0x0F : 0x07);
}

// -----------------------------------------------------------------------------
Artist *ArtistListView::getSelection()
{
    return m_playlist->getArtists()[getSelectedIndex()];
}

// -----------------------------------------------------------------------------
bool ArtistListView::handleIRR(IRRCODE code)
{
    if( ListView::handleIRR(code) )
    {
        return true;
    }
    switch( code )
    {
        case IRRemote::ST_REPT:
            // [ST/REPT]キー : 選択中のアーティストのアルバム選択画面に切り替える
            {
                AlbumListView *view = (AlbumListView *)(View::getView(AlbumListView::ID));
                view->setArtist(getSelection());
                View::show(AlbumListView::ID);
            }
            break;
        case IRRemote::ZERO:
            // [0]キー: プレイバック画面に切り替える
            View::show(PlaybackView::ID);
            break;
        default:
            return false;
    }
    return true;
}



////////////////////////////////////////////////////////////////////////////////
//  SSLine
////////////////////////////////////////////////////////////////////////////////
SSLine::SSLine()
{
    m_x[0] = random(0, 255);
    m_x[1] = random(0, 255);
    m_y[0] = random(0, 63);
    m_y[1] = random(0, 63);
    m_dx[0] = random(1, 8)*(random(0,1)? 1 : -1);
    m_dy[0] = random(1, 5)*(random(0,1)? 1 : -1);
    m_dx[1] = random(1, 8)*(random(0,1)? -1 : 1);
    m_dy[1] = random(1, 5)*(random(0,1)? -1 : 1);
}

// -----------------------------------------------------------------------------
SSLine& SSLine::operator = (SSLine& src)
{
    m_x[0] = src.m_x[0];
    m_x[1] = src.m_x[1];
    m_y[0] = src.m_y[0];
    m_y[1] = src.m_y[1];
    m_dx[0] = src.m_dx[0];
    m_dx[1] = src.m_dx[1];
    return *this;
}

// -----------------------------------------------------------------------------
void SSLine::move()
{
    for( int n = 0 ; n < 2 ; n++ )
    {
        int16_t x = m_x[n] + m_dx[n];
        if( x < 0 || x > 255 )
        {
            m_dx[n] = random(2, 8)*((m_dx[n] > 0)? -1 : 1);
            x = m_x[n] + m_dx[n];
        }
        m_x[n] = x;
        int16_t y = m_y[n] + m_dy[n];
        if( y < 0 || y > 63 )
        {
            m_dy[n] = random(2, 5)*((m_dy[n] > 0)? -1 : 1);
            y = m_y[n] + m_dy[n];
        }
        m_y[n] = y;
    }
}

// -----------------------------------------------------------------------------
void SSLine::draw(SSD1322 *oled, uint16_t color)
{
     int16_t x0 = m_x[0];
     int16_t y0 = m_y[0];
     int16_t x1 = m_x[1];
     int16_t y1 = m_y[1];
     int16_t dx = abs(x1-x0);
     int16_t dy = abs(y1-y0);
     int16_t sx, sy;
     if( x0 < x1 ){ sx = 1; }else{ sx = -1; }
     if( y0 < y1 ){ sy = 1; }else{ sy = -1; }
     int16_t err = dx - dy;

     while( true )
     {
          oled->drawPixel(x0, y0, color);
          if( x0 == x1 && y0 == y1 )
          {
               return;
          }
          int e2 = 2 * err;
          if( e2 > -dy )
          {
               err = err - dy;
               x0  = x0 + sx;
          }
          if( e2 < dx )
          {
               err = err + dx;
               y0  = y0 + sy;
          }
     }
}

////////////////////////////////////////////////////////////////////////////////
//  ScreenSaverView
////////////////////////////////////////////////////////////////////////////////
const uint16_t ScreenSaverView::COLOR_TABLE[ScreenSaverView::NUM_COLORS] = {
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05
};

// -----------------------------------------------------------------------------
ScreenSaverView::ScreenSaverView(SSD1322 *oled, Playlist *playlist)
    : View(oled, playlist, ScreenSaverView::ID), m_idle_timeout(300000),
    m_line_count(1), m_next_tick(0), m_color_index(0)
{
    // m_lines[0].setColor(COLOR_TABLE[m_color_index/MAX_LINE_NUM]);
}

// -----------------------------------------------------------------------------
void ScreenSaverView::init()
{
    EEPROM().read(0x0200, 4, (uint8_t *)&m_idle_timeout);
    m_idle_timeout = 60000 * (m_idle_timeout / 60000);
    if( m_idle_timeout > 99*60000 )
    {
        m_idle_timeout = 99 * 60000;
    }
    m_next_tick = millis() + m_idle_timeout;
}

// -----------------------------------------------------------------------------
void ScreenSaverView::refresh()
{
    m_line_count = 1;
    m_next_tick = millis() + UPDATE_INTERVAL;
    m_lines[0].draw(m_oled, COLOR_TABLE[m_color_index/MAX_LINE_NUM]);
}

// -----------------------------------------------------------------------------
void ScreenSaverView::update()
{
    if( millis() < m_next_tick )
    {
        return;
    }

    Rectangle r[2];
    if( m_line_count < MAX_LINE_NUM )
    {
        m_line_count++;
        m_lines[m_line_count-1] = m_lines[m_line_count-2];
    }
    else
    {
        m_lines[0].draw(m_oled, 0x00);
        m_lines[0].getBound(r[0]);
        for( int n = 0 ; n < m_line_count-1 ; n++ )
        {
            m_lines[n] = m_lines[n+1];
            m_lines[n].draw(m_oled, COLOR_TABLE[m_color_index/MAX_LINE_NUM]);
            m_lines[0].getBound(r[1]);
            r[1].unionWith(r[0]);
        }
        m_color_index = (m_color_index + 1) % (NUM_COLORS*MAX_LINE_NUM);
    }
    m_lines[m_line_count-1].move();
    m_lines[m_line_count-1].draw(m_oled, COLOR_TABLE[m_color_index/MAX_LINE_NUM]);
    m_lines[m_line_count-1].getBound(r[1]);
    r[1].unionWith(r[0]);

    // for( int n = 0 ; n < m_line_count ; n++ )
    // {
    //     m_lines[n].draw(m_oled);
    // }
    m_oled->invalidateRect(r[1]);
    m_next_tick += UPDATE_INTERVAL;
}

// -----------------------------------------------------------------------------
bool ScreenSaverView::handleIRR(IRRCODE code)
{
    if( isVisible() )
    {
        if( code )
        {
            View::show(PlaybackView::ID);
            m_next_tick = millis() + m_idle_timeout;
            return true;
        }
    }
    else
    {
        if( code )
        {
            m_next_tick = millis() + m_idle_timeout;
        }
        else
        {
            if( millis() > m_next_tick )
            {
                View::show(ID);
            }
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
void ScreenSaverView::setTimeoutValue(uint32_t t)
{
    m_idle_timeout = t;
    EEPROM().write(0x0200, 4, (uint8_t *)&m_idle_timeout);
}
        

////////////////////////////////////////////////////////////////////////////////
//  ConfigView
////////////////////////////////////////////////////////////////////////////////
const int16_t ConfigView::EDITCHAR_POS_X[2][12] = {
    {120, 127, 134, 141,    155, 162,    176, 183,    197, 204,    218, 225},  
    {120, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
const int16_t ConfigView::EDITCHAR_POS_Y[2] = {20, 40};
const int ConfigView::EDIT_LENGTH[2] = {12, 2};

// -----------------------------------------------------------------------------
ConfigView::ConfigView(SSD1322 *oled, Playlist *playlist)
    : View(oled, playlist, ConfigView::ID), m_edit_col(-1), m_edit_row(0)
{

}

// -----------------------------------------------------------------------------
void ConfigView::init()
{
    View::init();
    m_cursor_images.load("cursor.dat", 1);
}

// -----------------------------------------------------------------------------
void ConfigView::refresh()
{
    m_edit_row = 0;
    m_edit_col = -1;

    View::refresh();

    m_oled->drawString(0, 0, "設定", SSD1322::FONT_SMALL, 0x0F);
    m_oled->drawHzLine(0, 15, 256, 0x0F);

    ScreenSaverView *view = (ScreenSaverView *)View::getView(ScreenSaverView::ID);
    uint32_t t = view->getTimeoutValue() / 60000;   // msec -> 分単位に
    m_edit_value[1][0] = '0' + (t/10);
    m_edit_value[1][1] = '0' + (t%10);

    drawClock(false);
    drawScreenSaverTime(false);
}

// -----------------------------------------------------------------------------
void ConfigView::drawClock(bool force_redraw)
{
    bool focused = (m_edit_row == 0)? true : false;
    bool editing = ((m_edit_col >= 0)? true : false) && focused;

    m_oled->fillRect(0, EDITCHAR_POS_Y[0], 256, 16, 0x00);
    if( focused )
    {
        m_oled->drawImage(0, EDITCHAR_POS_Y[0], m_cursor_images.getImage(0));
    }
    else
    {
        m_oled->fillRect(0, EDITCHAR_POS_Y[0], 20, 16, 0x00);
    }
    m_oled->drawString(20, EDITCHAR_POS_Y[0], "時計合わせ", SSD1322::FONT_SMALL, focused? 0x0F : 0x07);
    if( editing )
    {
        for( int n = 0 ; n < EDIT_LENGTH[0] ; n++ )
        {
            m_oled->drawChar(EDITCHAR_POS_X[0][n], EDITCHAR_POS_Y[0], m_edit_value[0][n], SSD1322::FONT_SMALL, (n == m_edit_col)? 0x0F : 0x07);
        }
        m_oled->drawChar(EDITCHAR_POS_X[0][3]+EDITCHAR_WIDTH, EDITCHAR_POS_Y[0], '-', SSD1322::FONT_SMALL, 0x07);
        m_oled->drawChar(EDITCHAR_POS_X[0][5]+EDITCHAR_WIDTH, EDITCHAR_POS_Y[0], '-', SSD1322::FONT_SMALL, 0x07);
        m_oled->drawChar(EDITCHAR_POS_X[0][9]+EDITCHAR_WIDTH, EDITCHAR_POS_Y[0], ':', SSD1322::FONT_SMALL, 0x07);
    }
    else
    {
        m_oled->drawString(EDITCHAR_POS_X[0][0], EDITCHAR_POS_Y[0], m_clock, SSD1322::FONT_SMALL, focused? 0x0F : 0x07);
    }
    if( force_redraw )
    {
        m_oled->invalidateRect(0, EDITCHAR_POS_Y[0], 256, 16);
    }
}

// -----------------------------------------------------------------------------
void ConfigView::drawScreenSaverTime(bool force_redraw)
{
    bool focused = (m_edit_row == 1)? true : false;
    bool editing = ((m_edit_col >= 0)? true : false) && focused;

    m_oled->fillRect(0, EDITCHAR_POS_Y[1], 256, 16, 0x00);
    if( focused )
    {
        m_oled->drawImage(0, EDITCHAR_POS_Y[1], m_cursor_images.getImage(0));
    }
    m_oled->drawString(20, EDITCHAR_POS_Y[1], "スクリーンセーバ", SSD1322::FONT_SMALL, focused? 0x0F : 0x07);
    uint8_t col;
    for( int n = 0 ; n < EDIT_LENGTH[1] ; n++ )
    {
        if( editing )
        {
            col = (m_edit_col == n)? 0x0F : 0x07;
        }
        else if( focused )
        {
            col = 0x0F;
        }
        else
        {
            col = 0x07;
        }
        m_oled->drawChar(EDITCHAR_POS_X[1][n], EDITCHAR_POS_Y[1], m_edit_value[1][n], SSD1322::FONT_SMALL, col);
    }
    m_oled->drawString(141, 40, "分", SSD1322::FONT_SMALL, focused? 0x0F : 0x08);
    if( force_redraw )
    {
        m_oled->invalidateRect(0, EDITCHAR_POS_Y[1], 256, 16);
    }
}

// -----------------------------------------------------------------------------
void ConfigView::update()
{
    if( m_edit_row == 1 || m_edit_col < 0 )
    {
        // 時計の調整中でなければ、現在時刻表示を更新
        M41T62& rtc = M41T62::getInstance();
        rtc.update();
        char str[20];
        rtc.getAsText(str, false);
        if( strcmp(str, m_clock) )
        {
            strcpy(m_clock, str);
            drawClock(true);
        }
    }
}

// -----------------------------------------------------------------------------
void ConfigView::startEditing()
{
    if( m_edit_row == 0 )
    {
        m_edit_col = 2; // 西暦の上２桁は編集できない
        M41T62& rtc = M41T62::getInstance();
        rtc.update();
        rtc.getAsText(m_edit_value[0], true);
        drawClock(true);
    }
    else
    {
        m_edit_col = 0;
        ScreenSaverView *view = (ScreenSaverView *)View::getView(ScreenSaverView::ID);
        uint32_t t = view->getTimeoutValue() / 60000;   // msec -> 分単位に
        m_edit_value[1][0] = '0' + (t/10);
        m_edit_value[1][1] = '0' + (t%10);
        drawScreenSaverTime(true);
    }
}

// -----------------------------------------------------------------------------
void ConfigView::cancelEditing()
{
    m_edit_col = -1;
    ScreenSaverView *view = (ScreenSaverView *)View::getView(ScreenSaverView::ID);
    uint32_t t = view->getTimeoutValue() / 60000;
    m_edit_value[1][0] = '0' + (t/10);
    m_edit_value[1][1] = '0' + (t%10);

    drawClock(true);
    drawScreenSaverTime(true);
}

// -----------------------------------------------------------------------------
void ConfigView::input(int digit)
{
    if( m_edit_row == 0 )
    {
        bool accept = false;
        switch( m_edit_col )
        {
            case 2: // 「西暦年」の１０の位
            case 3: // 「西暦年」の１の位
                accept = true;
                break;
            case 4: // 「月」の１０の位
                accept = (digit < 2)? true : false;
                break;
            case 5: // 「月」の１の位
                if( m_edit_value[0][4] == '0' )
                {
                    accept = (digit > 0)? true : false;
                }
                else
                {
                    accept = (digit < 3)? true : false;
                }
                break;
            case 6: // 「日」の１０の位
                accept = (digit < 4)? true : false;
                break;
            case 7: // 「日」の１の位
                if( m_edit_value[0][6] == '0' )
                {
                    accept = (digit > 0)? true : false;
                }
                else if( m_edit_value[0][6] == '3' )
                {
                    accept = (digit == 0 || digit == 1)? true : false;
                }
                else
                {
                    accept = true;
                }
                break;
            case 8:     // 「時」の１０の位
                accept = (digit < 3)? true : false;
                break;
            case 9:     // 「時」の１の位
                if( m_edit_value[0][8] == '2' )
                {
                    accept = (digit < 4)? true : false;
                }
                else
                {
                    accept = true;
                }
                break;
            case 10:    // 「分」の１０の位
                accept = (digit < 6)? true : false;
                break;
            case 11:    // 「分」の１の位
                accept = true;
                break;
        }
        if( accept )
        {
            m_edit_value[0][m_edit_col] = '0' + digit;
            if( ++m_edit_col >= EDIT_LENGTH[0] )
            {
                m_edit_col = 2;
            }
            drawClock(true);
        }
    }
    else
    {
        m_edit_value[1][m_edit_col] = '0' + digit;
        if( ++m_edit_col >= EDIT_LENGTH[1] )
        {
            m_edit_col = 0;
        }
        drawScreenSaverTime(true);
    }
}

// -----------------------------------------------------------------------------
void ConfigView::acceptEditing()
{
    m_edit_col = -1;
    if( m_edit_row == 0 )
    {
        uint16_t year   = 2000 + 10*(m_edit_value[0][2] - '0') + (m_edit_value[0][3] - '0');
        uint16_t month  = 10*(m_edit_value[0][4] - '0') + (m_edit_value[0][5] - '0');
        uint16_t day    = 10*(m_edit_value[0][6] - '0') + (m_edit_value[0][7] - '0');
        uint16_t hour   = 10*(m_edit_value[0][8] - '0') + (m_edit_value[0][9] - '0');
        uint16_t minute = 10*(m_edit_value[0][10] - '0') + (m_edit_value[0][11] - '0');
        M41T62::getInstance().set(year, month, day, hour, minute);
        drawClock(true);
    }
    else
    {
        uint32_t t = 60000 * (10*(m_edit_value[1][0] - '0') + (m_edit_value[1][1] - '0'));
        ScreenSaverView *view = (ScreenSaverView *)View::getView(ScreenSaverView::ID);
        view->setTimeoutValue(t);
        drawScreenSaverTime(true);
    }
}

// -----------------------------------------------------------------------------
bool ConfigView::handleIRR(IRRCODE code)
{
    bool handled = false;
    switch( code )
    {
        case IRRemote::UP:
        case IRRemote::DOWN:
            m_edit_row = 1 - m_edit_row;
            cancelEditing();
            handled = true;
            break;
        case IRRemote::ST_REPT:
            if( m_edit_col >= 0 )
            {
                acceptEditing();
            }
            else
            {
                startEditing();
            }
            handled = true;
            break;
        default:
            if( m_edit_col >= 0 )
            {
                int d = IRRemote::codeToDigit(code);
                if( d >= 0 )
                {
                    input(d);
                    handled = true;
                }
            }
            else
            {
                View::show(PlaybackView::ID);
                handled = true;
            }
            break;
    }
    return handled;
}



////////////////////////////////////////////////////////////////////////////////
//  ClockView
////////////////////////////////////////////////////////////////////////////////
ClockView::ClockView(SSD1322 *oled, Playlist *playlist)
    : View(oled, playlist, ClockView::ID),
    m_month(0), m_day(0), m_hour(0), m_minute(0)
{

}

// -----------------------------------------------------------------------------
void ClockView::init()
{
    View::init();
    m_sseg_images.load("sevenseg.dat", 11);
    m_digit_images.load("calendar.dat", 13);
    m_week_images.load("week.dat", 9);
}

//
void ClockView::show()
{
    m_timeout = millis() + 10000;
    View::show();
}

// -----------------------------------------------------------------------------
void ClockView::refresh()
{
    m_blink = true;
    m_tick = millis() + 500;
    M41T62& rtc = M41T62::getInstance();
    rtc.update();
    uint16_t year, week, second;
    rtc.get(year, m_month, m_day, week, m_hour, m_minute, second);
    drawClock(m_hour, m_minute, m_blink);
    drawCalendar(m_month, m_day, week);
}

// -----------------------------------------------------------------------------
void ClockView::drawClock(uint16_t hour, uint16_t minute, bool blink)
{
    m_oled->fillRect(0, 7, 162, 50, 0x00);
    if( hour >= 10 )
    {
        m_oled->drawImage(2, 7, m_sseg_images.getImage(hour/10));
    }
    m_oled->drawImage(34, 7, m_sseg_images.getImage(hour%10));
    if( blink )
    {
        m_oled->drawImage(66, 7, m_sseg_images.getImage(10));
    }
    m_oled->drawImage(98, 7, m_sseg_images.getImage(minute/10));
    m_oled->drawImage(130, 7, m_sseg_images.getImage(minute%10));
}

// -----------------------------------------------------------------------------
void ClockView::drawCalendar(uint16_t month, uint16_t day, uint16_t week)
{
    m_oled->fillRect(174, 8, 80, 48, 0x00);
    if( month >= 10 )
    {
        m_oled->drawImage(174, 8, m_digit_images.getImage(month/10));
    }    
    m_oled->drawImage(190, 8, m_digit_images.getImage(month%10));
    m_oled->drawImage(206, 8, m_digit_images.getImage(10));
    if( day >= 10 )
    {
        m_oled->drawImage(222, 8, m_digit_images.getImage(day/10));
    }
    m_oled->drawImage(238, 8, m_digit_images.getImage(day%10));

    m_oled->drawImage(175, 32, m_week_images.getImage(7));
    m_oled->drawImage(201, 32, m_week_images.getImage(week));
    m_oled->drawImage(227, 32, m_week_images.getImage(8));
}

// -----------------------------------------------------------------------------
void ClockView::update()
{
    if( millis() > m_timeout )
    {
        View::show(PlaybackView::ID);
        return;
    }

    M41T62& rtc = M41T62::getInstance();
    rtc.update();
    uint16_t year, month, day, week, hour, minute, second;
    rtc.get(year, month, day, week, hour, minute, second);
    bool b = false;
    if( millis() >= m_tick )
    {
        m_blink = !m_blink;
        m_tick += 500;
        b = true;
    }
    if( b || hour != m_hour || minute != m_minute )
    {
        drawClock(hour, minute, m_blink);
        m_oled->invalidateRect(0, 7, 162, 50);
        m_hour = hour;
        m_minute = minute;
    }
    if( m_month != month || m_day != day )
    {
        drawCalendar(month, day, week);
        m_oled->invalidateRect(174, 8, 80, 48);
        m_month = month;
        m_day = day;
    }
}

// -----------------------------------------------------------------------------
bool ClockView::handleIRR(IRRCODE code)
{
    if( code == IRRemote::SIX )
    {
        View::show(ConfigView::ID);
    }
    else
    {
        View::show(PlaybackView::ID);
    }
    return true;
}



////////////////////////////////////////////////////////////////////////////////
//  PopupView
//  Volume(音量) : 0～20
//  Bass(低音)   : 0～10
//  Treble(高音) : 0～7 (0～5)
//
// +--------------------+
// | LABEL  ___________ |
// +--------------------+
//  width  : 1+1+2+36+2+80+2+1+1 = 126
//  height : 1+1+2+8+2+1+1       = 16
////////////////////////////////////////////////////////////////////////////////
PopupView::PopupView(SSD1322 *oled) : m_oled(oled), m_visible(false), m_prop_type(PROP_VOLUME)
{
    m_popup_rect.setRect(64, 48, 128, 16);
}

// -----------------------------------------------------------------------------
void PopupView::init()
{
    m_label_images.load("label_2.dat", 3);
}

// -----------------------------------------------------------------------------
void PopupView::show(uint8_t prop_type)
{
    m_prop_type = prop_type;
    m_visible = true;
    m_idle_timer = millis() + IDLE_TIMEOUT;
    m_oled->enablePopup(m_popup_rect);
    refresh();
}

// -----------------------------------------------------------------------------
void PopupView::hide()
{
    m_visible = false;
    Player().saveConfig();
    m_oled->disablePopup();
    View::getActiveView()->invalidate(true);
    // m_oled->invalidateRect(m_popup_rect);
}

// -----------------------------------------------------------------------------
void PopupView::update()
{
    if( m_visible )
    {
        if( millis() > m_idle_timer )
        {
            // 無操作状態が一定時間続いたらポップアップを消す
            hide();
        }
    }
}

// -----------------------------------------------------------------------------
void PopupView::refresh()
{
    m_oled->startDrawPopup();
    Rectangle rc = m_popup_rect;
    m_oled->fillRect(rc, 0x00);
    rc.inflate(-1, -1);
    m_oled->drawRect(rc, 0x0F);
    m_oled->drawImage(rc.left+4, rc.top+3, m_label_images.getImage(m_prop_type));
    drawBar(false); 
    m_oled->invalidateRect(m_popup_rect);   
    m_oled->endDrawPopup();
}

// -----------------------------------------------------------------------------
void PopupView::drawBar(bool refresh)
{
    int16_t mark = 0;
    switch( m_prop_type )
    {
        case PROP_VOLUME:
            mark = (int16_t)(Player().getVolume()*4);
            break;
        case PROP_BASS:
            mark = (int16_t)(Player().getBass()*8);
            break;
        case PROP_TREBLE:
            mark = (int16_t)(Player().getTreble()*16);
            break;
    }
    int16_t space = 80 - mark;
    int16_t x = m_popup_rect.left + 43;
    int16_t y = m_popup_rect.top + 4;
    if( refresh )
    {
        m_oled->startDrawPopup();
    }
    if( mark )
    {
        m_oled->fillRect(x, y, mark, 8, 0x0F);
    }
    if( space )
    {
        m_oled->fillRect(x+mark, y, space, 8, 0x07);
    }
    if( refresh )
    {
        m_oled->invalidateRect(x, y, 80, 8);
        m_oled->endDrawPopup();
    }
}

// -----------------------------------------------------------------------------
bool PopupView::handleIRR(IRRCODE code)
{
    // スクリーンセーバーが起動中なら、まずそっちに制御の機会を与える
    // （その結果、ビューがPlaybackViewに切り替わる）
    View::getView(ScreenSaverView::ID)->handleIRR(1);

    switch( code )
    {
        case IRRemote::VOL_UP:
        case IRRemote::VOL_DOWN:
            Player().setVolumeDelta((code == IRRemote::VOL_UP)? 1 : -1);
            if( !m_visible )
            {
                show(PROP_VOLUME);
            }
            else if( m_prop_type == PROP_VOLUME )
            {
                drawBar(true);
            }
            else
            {
                m_prop_type = PROP_VOLUME;
                refresh();
            }
            break;
        case IRRemote::EQ:
            if( !m_visible )
            {
                show(PROP_BASS);
            }
            else
            {
                if( m_prop_type == PROP_BASS )
                {
                    m_prop_type = PROP_TREBLE;
                }
                else
                {
                    m_prop_type = PROP_BASS;
                }
                refresh();
            }
            break;
        case IRRemote::UP:
        case IRRemote::DOWN:
            if( !m_visible )
            {
                return false;
            }
            if( m_prop_type == PROP_VOLUME )
            {
                Player().setVolumeDelta((code == IRRemote::UP)? 1 : -1);
            }
            if( m_prop_type == PROP_BASS )
            {
                Player().setBassDelta((code == IRRemote::UP)? 1 : -1);
            }
            if( m_prop_type == PROP_TREBLE )
            {
                Player().setTrebleDelta((code == IRRemote::UP)? 1 : -1);
            }
            drawBar(true);
            break;
        default:
            if( m_visible )
            {
                hide();
            }
            return false;
    }
    m_idle_timer = millis() + IDLE_TIMEOUT;
    return true;
}
