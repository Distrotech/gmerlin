/*****************************************************************
 
  qt.h
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/*
 *  The quicktime stuff is so complicated, that it is split into
 *  several files. So we need this header
 */

/* Forward declaration */

typedef struct qt_minf_s qt_minf_t;
typedef struct qt_trak_s qt_trak_t;

#define READ_VERSION_AND_FLAGS \
uint8_t version; \
if(!bgav_input_read_8(input, &version) || \
   !bgav_input_read_24_be(input, &(ret->flags))) \
  return 0;\
ret->version = version

/* Quicktime stuff */

/*
 *  The quicktime API is hidden to the public, so
 *  we can use simpler typenames here
 */

/* Generic Atom */

typedef struct
  {
  uint64_t size;
  uint64_t start_position;
  uint32_t fourcc;
  } qt_atom_header_t;

int bgav_qt_atom_read_header(bgav_input_context_t * input,
                             qt_atom_header_t * h);

void bgav_qt_atom_dump_header(int indent, qt_atom_header_t * h);

void bgav_qt_atom_skip(bgav_input_context_t * input,
                       qt_atom_header_t * h);

void bgav_qt_atom_skip_unknown(bgav_input_context_t * input,
                               qt_atom_header_t * h, uint32_t parent);

/* Reference movies (== Quicktime redirectors) */

typedef struct
  {
  uint32_t flags;
  uint32_t fourcc;
  uint32_t data_ref_size;
  uint8_t * data_ref;
  } qt_rdrf_t;

int bgav_qt_rdrf_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_rdrf_t * ret);

void bgav_qt_rdrf_free(qt_rdrf_t * r);
void bgav_qt_rdrf_dump(int indent, qt_rdrf_t * r);

typedef struct
  {
  qt_atom_header_t h;
  int has_rdrf;
  qt_rdrf_t rdrf;
  } qt_rmda_t;

int bgav_qt_rmda_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_rmda_t * ret);

void bgav_qt_rmda_free(qt_rmda_t * r);
void bgav_qt_rmda_dump(int indent, qt_rmda_t * r);

typedef struct
  {
  qt_atom_header_t h;

  int num_rmda;
  qt_rmda_t * rmda;
  } qt_rmra_t;

int bgav_qt_rmra_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_rmra_t * ret);

void bgav_qt_rmra_free(qt_rmra_t * r);

void bgav_qt_rmra_dump(int indent, qt_rmra_t * r);

/* tref */

typedef struct
  {
  int num_references;
  struct
    {
    uint32_t type;
    int num_tracks;
    uint32_t * tracks;
    } * references;
  } qt_tref_t;

int bgav_qt_tref_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_tref_t * ret);

void bgav_qt_tref_free(qt_tref_t * r);

void bgav_qt_tref_dump(int indent, qt_tref_t * r);


/*
 *  Time to sample
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;

  uint32_t num_entries;
  struct
    {
    uint32_t count;
    uint32_t duration;
    } * entries;
  } qt_stts_t;

int bgav_qt_stts_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_stts_t * ret);
void bgav_qt_stts_free(qt_stts_t * c);
void bgav_qt_stts_dump(int indent, qt_stts_t * c);

/*
 *  Sync samples
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t num_entries;
  uint32_t * entries;
  } qt_stss_t;

int bgav_qt_stss_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                  qt_stss_t * ret);
void bgav_qt_stss_free(qt_stss_t * c);
void bgav_qt_stss_dump(int indent, qt_stss_t * c);



/* esds */

typedef struct
  {
  qt_atom_header_t h;
  int      version;
  uint32_t flags;

  int	   decoderConfigLen;
  uint8_t  objectTypeId;
  uint8_t  streamType;
  uint32_t bufferSizeDB;
  uint32_t maxBitrate;
  uint32_t avgBitrate;
  uint8_t* decoderConfig;
  } qt_esds_t;

int bgav_qt_esds_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_esds_t * ret);

void bgav_qt_esds_free(qt_esds_t * ret);

void bgav_qt_esds_dump(int indent, qt_esds_t * e);

/* pasp */

typedef struct
  {
  qt_atom_header_t h;
  uint32_t hSpacing;
  uint32_t vSpacing;
  } qt_pasp_t;

int bgav_qt_pasp_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_pasp_t * ret);

void bgav_qt_pasp_dump(int indent, qt_pasp_t * e);

/* fiel */

typedef struct
  {
  qt_atom_header_t h;
  uint8_t fields;
  uint8_t detail;
  } qt_fiel_t;

int bgav_qt_fiel_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_fiel_t * ret);

void bgav_qt_fiel_dump(int indent, qt_fiel_t * f);

/* frma */

typedef struct
  {
  uint32_t fourcc;
  } qt_frma_t;

int bgav_qt_frma_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_frma_t * ret);

void bgav_qt_frma_dump(int indent, qt_frma_t * f);

/* enda */

typedef struct
  {
  uint16_t littleEndian;
  } qt_enda_t;

int bgav_qt_enda_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_enda_t * ret);

void bgav_qt_enda_dump(int indent, qt_enda_t * f);

/* chan */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  
  uint32_t mChannelLayoutTag;
  uint32_t mChannelBitmap;
  uint32_t mNumberChannelDescriptions;

  struct
    {
    uint32_t mChannelLabel;
    uint32_t mChannelFlags;
    float    mCoordinates[3];
    } * ChannelDescriptions;
  } qt_chan_t;

int bgav_qt_chan_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_chan_t * chan);
void bgav_qt_chan_dump(int indent, qt_chan_t * chan);

void bgav_qt_chan_get(qt_chan_t * chan, gavl_audio_format_t * format);

void bgav_qt_chan_free(qt_chan_t * chan);


/* wave */


typedef struct
  {
  /* Some codecs need the wave atom in raw form */
  uint8_t * raw;
  int raw_size;
    
  int has_frma;
  qt_frma_t frma;

  int has_enda;
  qt_enda_t enda;

  int has_esds;
  qt_esds_t esds;

  int num_user_atoms;
  uint8_t ** user_atoms;
  
  } qt_wave_t;

int bgav_qt_wave_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_wave_t * ret);

void bgav_qt_wave_dump(int indent, qt_wave_t * f);

void bgav_qt_wave_free(qt_wave_t * r);

/*
 *  Font table (used in 3gpp subtitles)
 */

typedef struct
  {
  uint16_t num_fonts;

  struct
    {
    uint16_t font_id;
    char font_name[256];
    } * fonts;
  } qt_ftab_t;

int bgav_qt_ftab_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_ftab_t * ret);

void bgav_qt_ftab_dump(int indent, qt_ftab_t * f);

void bgav_qt_ftab_free(qt_ftab_t * r);


/*
 *  Sample description
 */

typedef struct
  {
  /* Common members (20 bytes) */
  
  uint32_t fourcc;
  uint8_t reserved[6];
  uint16_t data_reference_index;
  uint16_t version;
  uint16_t revision_level;
  uint32_t vendor;
  int type; /* BGAV_STREAM_AUDIO, BGAV_STREAM_VIDEO or BGAV_STREAM_SUBTITLE_TEXT */
  union
    {
    struct
      {
      uint32_t temporal_quality;
      uint32_t spatial_quality;
      uint16_t width;
      uint16_t height;
      float horizontal_resolution;
      float vertical_resolution;
      uint32_t data_size;
      uint16_t frame_count; /* Frames / sample */
      char compressor_name[32];
      uint16_t depth;
      uint16_t ctab_id;
      int private_ctab;
      uint16_t ctab_size;
      bgav_palette_entry_t * ctab;

      qt_pasp_t pasp;
      int has_pasp;
      qt_fiel_t fiel;
      int has_fiel;
      } video;
    struct
      {
      /* 12 bytes */
      uint32_t num_channels;
      uint32_t bits_per_sample;
      uint16_t compression_id;
      uint16_t packet_size;
      int samplerate; /* Actually fixed32 */

      /* For Version 1 (CBR and VBR) 16 bytes */
      uint32_t samples_per_packet;
      uint32_t bytes_per_packet;
      uint32_t bytes_per_frame;
      uint32_t bytes_per_sample;

      /* For Version 2 (will be converted to version 1) */
      uint32_t formatSpecificFlags;
      
      /* Extended fields */

      int has_wave;
      qt_wave_t wave;

      int has_chan;
      qt_chan_t chan;
      
      } audio;
    struct
      {
      uint32_t displayFlags;
      uint32_t textJustification;
      uint16_t bgColor[3];
      uint16_t defaultTextBox[4];
      uint32_t scrpStartChar;              /*starting character position*/
      uint16_t scrpHeight;
      uint16_t scrpAscent;
      uint16_t scrpFont;
      uint16_t scrpFace;
      uint16_t scrpSize;
      uint16_t scrpColor[3];
      char font_name[256];
      } subtitle_qt;
    struct
      {
      uint32_t display_flags;
      uint8_t horizontal_justification;
      uint8_t vertical_justification;
      uint8_t back_color[4];
      uint16_t defaultTextBox[4];
      uint16_t start_char_offset;
      uint16_t end_char_offset;
      uint16_t font_id;
      uint8_t  style_flags;
      uint8_t  font_size;
      uint8_t  text_color[4];
      int has_ftab;
      qt_ftab_t ftab;
      } subtitle_tx3g;
    } format;
  qt_esds_t esds;
  int has_esds;


  
  /* Data for avc1 (offset relative to data) */

  int avcC_offset;
  int avcC_size;
  
  int has_SMI;
  
  } qt_sample_description_t ;

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t num_entries;
  
  struct
    {
    /* Raw data (must be passed to codec) */
    uint32_t data_size;
    uint8_t * data;
    /* Parsed data */
    qt_sample_description_t desc;
    } * entries;
  
  } qt_stsd_t;

int bgav_qt_stsd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stsd_t * ret);

void bgav_qt_stsd_free(qt_stsd_t * c);
int bgav_qt_stsd_finalize(qt_stsd_t * c, qt_trak_t * trak,
                          const bgav_options_t * opt);

void bgav_qt_stsd_dump(int indent, qt_stsd_t * c);


/*
 * Sample to chunk
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t num_entries;

  struct
    {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t sample_description_id;
    } *
  entries;
  } qt_stsc_t;

int bgav_qt_stsc_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stsc_t * ret);
void bgav_qt_stsc_free(qt_stsc_t * c);
void bgav_qt_stsc_dump(int indent, qt_stsc_t * c);

/*
 *  Sample size table
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t sample_size;
  uint32_t num_entries;
  uint32_t * entries;
  } qt_stsz_t;

int bgav_qt_stsz_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stsz_t * ret);
void bgav_qt_stsz_free(qt_stsz_t * c);
void bgav_qt_stsz_dump(int indent, qt_stsz_t * c);

/*
 *  Chunk offset table
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t num_entries;
  uint64_t * entries;
  } qt_stco_t;

int bgav_qt_stco_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stco_t * ret);
int bgav_qt_stco_read_64(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stco_t * ret);
void bgav_qt_stco_free(qt_stco_t * c);
void bgav_qt_stco_dump(int indent, qt_stco_t * c);

/*
 *  Sample table
 */

typedef struct
  {
  qt_atom_header_t h;

  qt_stts_t stts;
  qt_stss_t stss;
  qt_stsd_t stsd;
  qt_stsz_t stsz;
  qt_stsc_t stsc;
  qt_stco_t stco;
  } qt_stbl_t;

int bgav_qt_stbl_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stbl_t * ret, qt_minf_t * minf);
void bgav_qt_stbl_free(qt_stbl_t * c);
void bgav_qt_stbl_dump(int indent, qt_stbl_t * c);

/*
 * Data reference
 */

typedef struct
  {
  uint32_t size;
  uint32_t type;
  int version;
  uint32_t flags;
  uint8_t * data_reference;
  } qt_dref_table_t;


typedef struct
  {
  int version;
  uint32_t flags;
  uint32_t table_size;
  qt_dref_table_t * table;
  } qt_dref_t;

int bgav_qt_dref_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_dref_t * dref);
void bgav_qt_dref_free(qt_dref_t * dref);
void bgav_qt_dref_dump(int indent, qt_dref_t * c);


/*
 *  dinf
 */

typedef struct
  {
  int has_dref;
  qt_dref_t dref;
  } qt_dinf_t;

int bgav_qt_dinf_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_dinf_t * dinf);
void bgav_qt_dinf_free(qt_dinf_t * dinf);
void bgav_qt_dinf_dump(int indent, qt_dinf_t * c);

/*
 * Media handler
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t component_type;
  uint32_t component_subtype;
  uint32_t component_manufacturer;
  uint32_t component_flags;
  uint32_t component_flag_mask;
  char * component_name;
  } qt_hdlr_t;

int bgav_qt_hdlr_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_hdlr_t * ret);

void bgav_qt_hdlr_free(qt_hdlr_t * h);
void bgav_qt_hdlr_dump(int indent, qt_hdlr_t * h);

/*
 *  Generic media info
 */

typedef struct
  {
  int version;
  uint32_t flags;
  uint16_t graphics_mode;
  uint16_t opcolor[3];
  uint16_t balance;
  uint16_t reserved;
  } qt_gmin_t;

int bgav_qt_gmin_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_gmin_t * ret);

void bgav_qt_gmin_free(qt_gmin_t * g);
void bgav_qt_gmin_dump(int indent, qt_gmin_t * g);

/*
 *  Generic media header
 */

typedef struct
  {
  int has_gmin;
  qt_gmin_t gmin;

  int has_text; /* We don't parse this just check if it's there */
  } qt_gmhd_t;

int bgav_qt_gmhd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_gmhd_t * ret);
void bgav_qt_gmhd_free(qt_gmhd_t * g);
void bgav_qt_gmhd_dump(int indent, qt_gmhd_t * g);

/*
 *  Null media header (used for MP4 subtitles)
 */

typedef struct
  {
  int version;
  uint32_t flags;
  } qt_nmhd_t;

int bgav_qt_nmhd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_nmhd_t * ret);
void bgav_qt_nmhd_free(qt_nmhd_t * n);
void bgav_qt_nmhd_dump(int indent, qt_nmhd_t * n);


/*
 *  elst
 */

typedef struct
  {
  int version;
  uint32_t flags;
  uint32_t num_entries;
  
  struct
    {
    uint32_t duration;
    uint32_t media_time;
    float media_rate;
    } * table;
  } qt_elst_t;

int bgav_qt_elst_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_elst_t * ret);
void bgav_qt_elst_free(qt_elst_t * g);
void bgav_qt_elst_dump(int indent, qt_elst_t * e);

/*
 *  edts
 */

typedef struct
  {
  int has_elst;
  qt_elst_t elst;
  } qt_edts_t;


int bgav_qt_edts_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_edts_t * ret);
void bgav_qt_edts_free(qt_edts_t * g);
void bgav_qt_edts_dump(int indent, qt_edts_t * e);

/*
 *  Media information
 */

struct qt_minf_s
  {
  qt_atom_header_t h;
  qt_stbl_t stbl;
  qt_hdlr_t hdlr;

  int has_dinf;
  qt_dinf_t dinf;
  
  int has_vmhd;
  int has_smhd;
  
  int has_gmhd;
  qt_gmhd_t gmhd;

  int has_nmhd;
  qt_nmhd_t nmhd;

  };

int bgav_qt_minf_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_minf_t * ret);

void bgav_qt_minf_free(qt_minf_t * h);

void bgav_qt_minf_dump(int indent, qt_minf_t * h);

/*
 *  Media header
 */

typedef struct
  {
  qt_atom_header_t h;

  int version;
  uint32_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  uint16_t language;
  uint16_t quality;
  } qt_mdhd_t;

int bgav_qt_mdhd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_mdhd_t * ret);

void bgav_qt_mdhd_free(qt_mdhd_t * c);
void bgav_qt_mdhd_dump(int indent, qt_mdhd_t * c);

int bgav_qt_mdhd_get_language(qt_mdhd_t * c, char * ret);

/*
 *  Media
 */

typedef struct
  {
  qt_atom_header_t h;

  qt_mdhd_t mdhd;
  qt_hdlr_t hdlr;
  qt_minf_t minf;
  } qt_mdia_t;

int bgav_qt_mdia_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_mdia_t * ret);

void bgav_qt_mdia_free(qt_mdia_t * c);
void bgav_qt_mdia_dump(int indent, qt_mdia_t * c);

/*
 *  Track header
 */

typedef struct
  {
  qt_atom_header_t h;

  uint32_t version;
  uint32_t flags;
  uint64_t creation_time;
  uint64_t modification_time;
  
  uint32_t track_id;
  uint32_t reserved1;
  uint64_t duration;
  uint8_t reserved2[8];
  uint16_t layer;
  uint16_t alternate_group;
  float volume;
  uint16_t reserved3;
  float matrix[9];
  float track_width;
  float track_height;
  } qt_tkhd_t;

int bgav_qt_tkhd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_tkhd_t * ret);

void bgav_qt_tkhd_free(qt_tkhd_t * c);
void bgav_qt_tkhd_dump(int indent, qt_tkhd_t * c);

/* udta atom */

typedef struct
  {
  int have_ilst;
  qt_atom_header_t h;
  /*
   *   We support all strings, which begin with the
   *   (C)-Character, from the Quicktime Spec
   */

  char * cpy; /* Copyright                        */
  char * day; /* Date                             */
  char * dir; /* Director                         */
  char * ed1; /* 1st edit date and description    */
  char * ed2; /* 2nd edit date and description    */
  char * ed3; /* 3rd edit date and description    */
  char * ed4; /* 4th edit date and description    */
  char * ed5; /* 5th edit date and description    */
  char * ed6; /* 6th edit date and description    */
  char * ed7; /* 7th edit date and description    */
  char * ed8; /* 8th edit date and description    */
  char * ed9; /* 9th edit date and description    */
  char * fmt; /* Format (Computer generated etc.) */  
  char * inf; /* Information about the movie      */
  char * prd; /* Producer                         */
  char * prf; /* Performers                       */
  char * rec; /* Hard- software requirements      */
  char * src; /* Credits for movie Source         */
  char * wrt; /* Writer                           */

  /* Additional stuff from OpenQuicktime (not in the QT spec I have) */

  char * nam; /* Name            */
  char * ART; /* Artist          */
  char * alb; /* Album           */
  char * enc; /* Encoded by      */
  char * trk; /* Track           */
  char * cmt; /* Comment         */
  char * aut; /* Author          */
  char * com; /* Composer        */
  char * des; /* Description     */
  char * dis; /* Disclaimer      */
  char * gen; /* Genre           */
  char * hst; /* Host computer   */
  char * mak; /* Make (??)       */
  char * mod; /* Model (??)      */
  char * ope; /* Original Artist */
  char * PRD; /* Product         */
  char * swr; /* Software        */
  char * wrn; /* Warning         */
  char * url; /* URL link        */

  uint16_t trkn;

  } qt_udta_t;

int bgav_qt_udta_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_udta_t * ret);

void bgav_qt_udta_free(qt_udta_t * udta);
void bgav_qt_udta_dump(int indent, qt_udta_t * udta);

/*
 *  Track atom
 */

struct qt_trak_s
  {
  qt_atom_header_t h;
  qt_mdia_t mdia;
  qt_tkhd_t tkhd;

  int has_udta;
  qt_udta_t udta;

  int has_edts;
  qt_edts_t edts;

  int has_tref;
  qt_tref_t tref;
  
  int is_chapter_track;
  };

int bgav_qt_trak_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_trak_t * ret);
void bgav_qt_trak_free(qt_trak_t * c);

void bgav_qt_trak_dump(int indent, qt_trak_t * c);

/* Infos about the track */

int64_t bgav_qt_trak_samples(qt_trak_t * c);

int64_t bgav_qt_trak_chunks(qt_trak_t * c);

int64_t bgav_qt_trak_tics(qt_trak_t * c); /* Duration in timescale tics */

/*
 *  Movie header
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint64_t creation_time;
  uint64_t modification_time;
  uint32_t time_scale;
  uint64_t duration;
  float preferred_rate;
  float preferred_volume;
  uint8_t reserved[10];
  float matrix[9];
  uint32_t preview_time;
  uint32_t preview_duration;
  uint32_t poster_time;
  uint32_t selection_time;
  uint32_t selection_duration;
  uint32_t current_time;
  uint32_t next_track_id;
  
  } qt_mvhd_t;

void bgav_qt_mvhd_dump(int indent, qt_mvhd_t * c);

int bgav_qt_mvhd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_mvhd_t * ret);
void bgav_qt_mvhd_free(qt_mvhd_t * c);


/*
 *  moov atom
 */

typedef struct
  {
  qt_atom_header_t h;
  qt_mvhd_t mvhd;
  
  qt_udta_t udta;
  int has_udta;
  
  int num_tracks;
  qt_trak_t * tracks;

  /* Reference movie */
  qt_rmra_t rmra;
  int has_rmra;
  } qt_moov_t;

int bgav_qt_moov_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_moov_t * ret);

/* Compressed movie header */

int bgav_qt_cmov_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_moov_t * ret);

void bgav_qt_moov_free(qt_moov_t * c);

void bgav_qt_moov_dump(int indent, qt_moov_t * c);

int bgav_qt_is_chapter_track(qt_moov_t * moov, qt_trak_t * trak);


/*
 *  Quicktime specific utilities
 */

int bgav_qt_read_fixed32(bgav_input_context_t * ctx,
                         float * ret);

int bgav_qt_read_fixed16(bgav_input_context_t * ctx,
                         float * ret);

/* language/charset support */

int bgav_qt_get_language(int mac_code, char * ret);
const char * bgav_qt_get_charset(int mac_code);

