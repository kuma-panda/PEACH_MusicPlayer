#include <Arduino.h>
#include <new.h>
#include <SD.h>
#include <Wire.h>
#include "playlist.h"
#include "eeprom_24lc.h"

////////////////////////////////////////////////////////////////////////////////
//  Song
////////////////////////////////////////////////////////////////////////////////
Song::Song(Album *album) : m_album(album), m_length(0), m_track_index(0)
{
    memset(m_title, 0, sizeof(m_title));
    memset(m_filename, 0, sizeof(m_filename));
}

// -----------------------------------------------------------------------------
void Song::load(File f)
{
    f.read(&m_track_index, sizeof(m_track_index));
    f.read(&m_length, sizeof(m_length));
    f.read(m_filename, sizeof(m_filename));
    f.read(m_title, sizeof(m_title));
}

////////////////////////////////////////////////////////////////////////////////
//  Album
////////////////////////////////////////////////////////////////////////////////
Album::Album(Artist *artist) : m_artist(artist), 
    m_id(0), m_total_length(0), m_year(0), m_current_index(0)
{
    memset(m_title, 0, sizeof(m_title));
}

//------------------------------------------------------------------------------
void Album::load(File f)
{
    f.read(&m_id, sizeof(m_id));
    f.read(m_title, sizeof(m_title));
    f.read(&m_year, sizeof(m_year));
    f.read(&m_total_length, sizeof(m_total_length));
    uint16_t num_tracks;
    f.read(&num_tracks, sizeof(num_tracks));
    m_songs.alloc(num_tracks);
    for( uint16_t i = 0 ; i < num_tracks ; i++ )
    {
        Song *s = new Song(this);
        m_songs.push_back(s);
        s->load(f);
    }
}

// -----------------------------------------------------------------------------
void Album::seekFirst()
{
    m_current_index = 0;
}

// -----------------------------------------------------------------------------
void Album::seekNext()
{
    if( hasNext() )
    {
        m_current_index++;
    }
}

// -----------------------------------------------------------------------------
bool Album::hasNext()
{
    return m_current_index < (m_songs.size() - 1);

}

// -----------------------------------------------------------------------------
void Album::seekPrev()
{
    if( m_current_index > 0 )
    {
        m_current_index--;
    }
}

// -----------------------------------------------------------------------------
void Album::seekTo(uint16_t index)
{
    if( index < m_songs.size() )
    {
        m_current_index = index;
    }
}


////////////////////////////////////////////////////////////////////////////////
//  Artist
////////////////////////////////////////////////////////////////////////////////
Artist::Artist() : m_id(0), m_selected_album_id(0)
{
    memset(m_name, 0, sizeof(m_name));
}

// -----------------------------------------------------------------------------
void Artist::load(File f)
{
    f.read(&m_id, sizeof(m_id));
    f.read(m_name, sizeof(m_name));
    uint16_t num_albums;
    f.read(&num_albums, sizeof(num_albums));
    m_albums.alloc(num_albums);
    for( uint16_t i = 0 ; i < num_albums ; i++ )
    {
        Album *a = new Album(this);
        m_albums.push_back(a);
        a->load(f);
    }
}

// -----------------------------------------------------------------------------
Album *Artist::getAlbumByID(uint16_t album_id)
{
    for( uint16_t i = 0 ; i < m_albums.size() ; i++ )
    {
        if( m_albums[i]->getID() == album_id )
        {
            return m_albums[i];
        }
    }
    return NULL;
}

// -----------------------------------------------------------------------------
Album *Artist::getSelectedAlbum()
{
    return getAlbumByID(m_selected_album_id);
}

// -----------------------------------------------------------------------------
void Artist::selectAlbumByID(uint16_t album_id)
{
    Album *target = getAlbumByID(album_id);
    if( !target )
    {
        // 指定されたアルバムが見つからない場合は、リストの先頭のアルバムを選択
        target = m_albums[0];
    }
    m_selected_album_id = target->getID();
    target->seekFirst();
}

// -----------------------------------------------------------------------------
uint16_t Artist::getIndexOfAlbum(Album *album)
{
    for( uint16_t i = 0 ; i < m_albums.size() ; i++ )
    {
        if( m_albums[i]->getID() == album->getID() )
        {
            return i;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  Playlist
////////////////////////////////////////////////////////////////////////////////
Playlist::Playlist() : m_selected_artist_id(0)
{
}

// -----------------------------------------------------------------------------
void Playlist::load(const char *path)
{
    File f = SD.open(path);
    if( !f )
    {
        Serial.print("Cannot open ");
        Serial.println(path);
        while(true){}
    }

    uint16_t num_artists;
    f.read(&num_artists, sizeof(num_artists));
    m_artists.alloc(num_artists);
    for( uint16_t i = 0 ; i < num_artists ; i++ )
    {
        Artist *a = new Artist();
        m_artists.push_back(a);
        a->load(f);
    }
    Serial.print(num_artists, DEC);
    Serial.println(" artist(s) successfully loaded");

    // EEPROM から読み込む
    // +00 +01 : アーティストID
    // +02 +03 : アルバムID
    // +04     : チェックサム
    uint8_t config[5];
    EEPROM().read(0x0000, 5, config);
    uint8_t sum = 0;
    for( int i = 0 ; i < 5 ; i++ )
    {
        sum += config[i];
    }
    if( sum != 0xFF )
    {
        Serial.println("EEPROM Read Error (0x0000 - 0x0004) -- checksum unmatched.");
        selectArtistByID(0);
    }
    else
    {
        uint16_t *p = (uint16_t *)config;
        uint16_t artist_id = *p;
        uint16_t album_id  = *(p+1);
        selectArtistByID(artist_id);
        Artist *artist = getSelectedArtist();
        artist->selectAlbumByID(album_id);
    }
}

// -----------------------------------------------------------------------------
void Playlist::save()
{
    uint8_t config[5];
    uint16_t *p = (uint16_t *)config;
    Artist *artist = getSelectedArtist();
    *p = artist->getID();
    Album *album = artist->getSelectedAlbum();
    *(p+1) = album->getID();
    uint8_t sum = 0x00;
    for( int i = 0 ; i < 4 ; i++ )
    {
        sum += config[i];
    }
    config[4] = 0xFF - sum;
    EEPROM().write(0x0000, 5, config);
}

// -----------------------------------------------------------------------------
Artist *Playlist::getArtistByID(uint16_t artist_id)
{
    for( uint16_t i = 0 ; i < m_artists.size() ; i++ )
    {
        if( m_artists[i]->getID() == artist_id )
        {
            return m_artists[i];
        }
    }
    return NULL;
}

// -----------------------------------------------------------------------------
Artist *Playlist::getSelectedArtist()
{
    return getArtistByID(m_selected_artist_id);
}

// -----------------------------------------------------------------------------
void Playlist::selectArtistByID(uint16_t artist_id)
{
    Artist *target = getArtistByID(artist_id);
    if( !target )
    {
        // 指定されたアーティストが見つからない場合は、リストの先頭のアーティストを選択
        target = m_artists[0];
    }
    m_selected_artist_id = target->getID();
    target->selectAlbumByID();
}

// -----------------------------------------------------------------------------
uint16_t Playlist::getIndexOfArtist(Artist *artist)
{
    for( uint16_t i = 0 ; i < m_artists.size() ; i++ )
    {
        if( m_artists[i]->getID() == artist->getID() )
        {
            return i;
        }
    }
    return 0;
}
