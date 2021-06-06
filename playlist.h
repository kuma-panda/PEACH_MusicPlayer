#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <Arduino.h>
#include <SD.h>

// -----------------------------------------------------------------------------
template <typename T>
class Vector
{
    private:
        uint16_t m_capacity;
        uint16_t m_size;
        T       *m_buffer;
    public:
        Vector() : m_capacity(0), m_size(0), m_buffer(NULL){
        }
        void alloc(uint16_t capacity){
            if( !m_buffer )
            {
                m_capacity = capacity;
                m_buffer = new T[capacity];
            }
        }
        uint16_t capacity(){
            return m_capacity;
        }
        void clear(){
            m_size = 0;
        }
        void push_back(const T& t){
            m_buffer[m_size++] = t;
        }
        T& front(){
            return m_buffer[0];
        }
        T& back(){
            return m_buffer[m_size-1];
        }
        T& at(uint16_t index){
            return m_buffer[index];
        }
        T& operator[](int index){
            return m_buffer[index];
        }
        uint16_t size(){
            return m_size;
        }
};

// -----------------------------------------------------------------------------
class Album;
class Song
{
    private:
        enum{MAX_TITLE_LENGTH = 128};
        enum{MAX_FILENAME_LENGTH = 64};
        Album *m_album;
        uint16_t m_length;
        uint16_t m_track_index;
        char m_title[MAX_TITLE_LENGTH];
        char m_filename[MAX_FILENAME_LENGTH];
    public:
        Song(Album *album);
        void load(File f);
        Album *getAlbum(){ return m_album; }
        uint16_t getTrackIndex(){ return m_track_index; }
        uint16_t getLength(){ return m_length; }
        const char *getTitle(){ return m_title; }
        const char *getFileName(){ return m_filename; }
};

// -----------------------------------------------------------------------------
class Artist;
class Album
{
    private:
        enum{MAX_TITLE_LENGTH = 128};
        Artist        *m_artist;
        uint16_t       m_id;
        uint16_t       m_total_length;
        uint16_t       m_year;
        uint16_t       m_current_index;
        Vector<Song *> m_songs;
        char           m_title[MAX_TITLE_LENGTH];
    public:
        Album(Artist *artist);
        void            load(File f);
        Artist         *getArtist(){ return m_artist; }
        uint16_t        getID(){ return m_id; }
        const char     *getTitle(){ return m_title; }
        uint16_t        getTotalLength(){ return m_total_length; }
        uint16_t        getYear(){ return m_year; }
        uint16_t        getSongCount(){ return m_songs.size(); }
        Vector<Song *>& getSongs(){ return m_songs; }
        void            seekFirst();
        void            seekNext();
        bool            hasNext();
        void            seekPrev();
        void            seekTo(uint16_t index); 
        Song           *getCurrentSong(){ return m_songs.at(m_current_index); }
};

// -----------------------------------------------------------------------------
class Artist
{
    private:
        enum{MAX_NAME_LENGTH = 128};
        uint16_t        m_id;
        Vector<Album *> m_albums;
        char            m_name[MAX_NAME_LENGTH];
        uint16_t        m_selected_album_id;
    public:
        Artist();
        void             load(File f);
        uint16_t         getID(){ return m_id; }
        const char      *getName(){ return m_name; }
        uint16_t         getAlbumCount(){ return m_albums.size(); }
        Vector<Album *>& getAlbums(){ return m_albums; }
        Album           *getAlbumByID(uint16_t album_id);
        Album           *getSelectedAlbum();
        void             selectAlbumByID(uint16_t album_id=0);
        uint16_t         getIndexOfAlbum(Album *album);
};

// -----------------------------------------------------------------------------
class Playlist
{
    private:
        Vector<Artist *> m_artists;
        uint16_t         m_selected_artist_id;
    public:
        Playlist();
        void              load(const char *path);
        void              save();
        uint16_t          getArtistCount(){ return m_artists.size(); }
        Vector<Artist *>& getArtists(){ return m_artists; }
        Artist           *getArtistByID(uint16_t artist_id);
        Artist           *getSelectedArtist();
        void              selectArtistByID(uint16_t artist_id=0);
        uint16_t          getIndexOfArtist(Artist *artist);
};

#endif
