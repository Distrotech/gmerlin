/*****************************************************************
 
  qt.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

void bgav_qt_atom_dump_header(qt_atom_header_t * h);

void bgav_qt_atom_skip(bgav_input_context_t * input,
                       qt_atom_header_t * h);

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

typedef struct
  {
  qt_atom_header_t h;
  qt_rdrf_t rdrf;
  } qt_rmda_t;

int bgav_qt_rmda_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_rmda_t * ret);

void bgav_qt_rmda_free(qt_rmda_t * r);

typedef struct
  {
  qt_atom_header_t h;
  qt_rmda_t rmda;
  } qt_rmra_t;

int bgav_qt_rmra_read(qt_atom_header_t * h,
                      bgav_input_context_t * ctx, qt_rmra_t * ret);

void bgav_qt_rmra_free(qt_rmra_t * r);

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

/* Esds */

typedef struct
  {
  qt_atom_header_t h;
  int      version;
  uint32_t flags;

  int	   decoderConfigLen;
  uint8_t  objectTypeId;
  uint8_t  streamType;
  int      bufferSizeDB;
  int32_t  maxBitrate;
  int32_t  avgBitrate;
  uint8_t* decoderConfig;
  } qt_esds_t;

int bgav_qt_esds_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_esds_t * ret);

void bgav_qt_esds_free(qt_esds_t * ret);

/*
 *  Sample description
 */

typedef struct
  {
  /* Common members (20 bytes) */
  
  uint32_t fourcc;
  char reserved[6];
  uint16_t data_reference_index;
  uint16_t version;
  uint16_t revision_level;
  uint32_t vendor;
  int type; /* BGAV_STREAM_AUDIO or BGAV_STREAM_VIDEO */
  union
    {
    struct
      {
      uint32_t temporal_quality;
      uint32_t spatial_quality;
      int16_t width;
      int16_t height;
      float horizontal_resolution;
      float vertical_resolution;
      uint32_t data_size;
      uint16_t frame_count; /* Frames / sample */
      char compressor_name[32];
      int16_t depth;
      int16_t ctab_id;
      int private_ctab;
      uint16_t ctab_size;
      bgav_palette_entry_t * ctab;
      } video;
    struct
      {
      /* 12 bytes */
      uint16_t num_channels;
      uint16_t bits_per_sample;
      uint16_t compression_id;
      uint16_t packet_size;
      int samplerate; /* Actually fixed32 */
      /* For Version 1 (CBR and VBR) 16 bytes */
                  
      uint32_t samples_per_packet;
      uint32_t bytes_per_packet;
      uint32_t bytes_per_frame;
      uint32_t bytes_per_sample;

      /* Extended fields */

      int has_wave_atom;
      struct
        {
        uint8_t * data;
        int size;
        } wave_atom;
      } audio;
    } format;
  qt_esds_t esds;
  int has_esds;
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
    int data_size;
    uint8_t * data;
    /* Parsed data */
    qt_sample_description_t desc;
    } * entries;
  
  } qt_stsd_t;

int bgav_qt_stsd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stsd_t * ret);

void bgav_qt_stsd_free(qt_stsd_t * c);
int bgav_qt_stsd_finalize(qt_stsd_t * c, qt_trak_t * trak);

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

/*
 *  Chunk offset table
 */

typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  int num_entries;
  uint64_t * entries;
  } qt_stco_t;

int bgav_qt_stco_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stco_t * ret);
int bgav_qt_stco_read_64(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_stco_t * ret);
void bgav_qt_stco_free(qt_stco_t * c);

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

/*
 *  Media information
 */

struct qt_minf_s
  {
  qt_atom_header_t h;
  qt_stbl_t stbl;
  qt_hdlr_t hdlr;
  
  int has_vmhd;
  int has_smhd;
  
  };

int bgav_qt_minf_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_minf_t * ret);

void bgav_qt_minf_free(qt_minf_t * h);

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

/*
 *  Track header
 */

typedef struct
  {
  qt_atom_header_t h;

  uint32_t version;
  uint32_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  
  uint32_t track_id;
  uint32_t reserved1;
  uint32_t duration;
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


/*
 *  Track atom
 */

struct qt_trak_s
  {
  qt_atom_header_t h;
  qt_mdia_t mdia;
  qt_tkhd_t tkhd;
  };

int bgav_qt_trak_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_trak_t * ret);
void bgav_qt_trak_free(qt_trak_t * c);

/*
 *  Movie header
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
  float preferred_rate;
  float preferred_volume;
  char reserved[10];
  float matrix[9];
  uint32_t preview_time;
  uint32_t preview_duration;
  uint32_t poster_time;
  uint32_t selection_time;
  uint32_t selection_duration;
  uint32_t current_time;
  uint32_t next_track_id;
  
  } qt_mvhd_t;

int bgav_qt_mvhd_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_mvhd_t * ret);
void bgav_qt_mvhd_free(qt_mvhd_t * c);

/* udta atom */

typedef struct
  {
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
  } qt_udta_t;

int bgav_qt_udta_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_udta_t * ret);

void bgav_qt_udta_free(qt_udta_t * udta);
void bgav_qt_udta_dump(qt_udta_t * udta);

/*
 *  moov atom
 */

typedef struct
  {
  qt_atom_header_t h;
  qt_mvhd_t mvhd;
  
  qt_udta_t udta;
  
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

/*
 *  Quicktime specific utilities
 */

int bgav_qt_read_fixed32(bgav_input_context_t * ctx,
                         float * ret);

int bgav_qt_read_fixed16(bgav_input_context_t * ctx,
                         float * ret);

