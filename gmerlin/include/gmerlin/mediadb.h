typedef enum
  {
  BG_MEDIA_AUDIO = (1<<0),
  BG_MEDIA_VIDEO = (1<<1),
  BG_MEDIA_PHOTO = (1<<2),
  } bg_media_type_t;

typedef enum
  {
  BG_MEDIA_ACCESS_FILE,
  BG_MEDIA_ACCESS_DEVICE,
  BG_MEDIA_ACCESS_URL,
  // BG_MEDIA_ACCESS_ONDEMAND?
  } bg_media_access_t;

typedef struct
  {
  int day;   // 1 - 31 (0 = unknown)
  int month; // 1 - 12 (0 = unknown)
  int year;  // 2013
  } bg_media_date_t;

typedef struct
  {
  int64_t id;

  char * path; // Relative to db file
  time_t mtime;
  int64_t size;
  int type;
  int64_t dir_id;
  } bg_media_file_t;

/* Directory on the file system */
typedef struct
  {
  int64_t id;

  char * path;   // Relative to db file
  int scan_type; // ORed flags if BG_MEDIA_* types
  } bg_media_directory_t;

/* Audio track */
typedef struct
  {
  int64_t id;

  int64_t album_id;
  int64_t file_id;
  int64_t artist_id;
  int64_t genre_id;
  int64_t album_id;

  char * title;
  gavl_time_t duration;
  char * format;
  char * bitrate;
  bg_media_date_t date;
  int track;

  /* Secondary variables */
  char * genre;
  char * album;
  char * albumartist;
  } bg_media_audio_t;

/* Video */
typedef struct
  {
  int64_t id;

  char * title;
  char * genre;
  int year;
  gavl_time_t duration;
  int64_t file_id;
  } bg_media_video_t;

/* Music Album */
typedef struct
  {
  int64_t id;
  char * artist;
  char * title;
  int tracks;
  } bg_media_musicalbum_t;

/* Playlist */
typedef struct
  {
  
  } bg_playlist_t;

