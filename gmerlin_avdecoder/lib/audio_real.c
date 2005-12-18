/*****************************************************************
 
  audio_real.c
 
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

#include <avdec_private.h>

#include <limits.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <codecs.h>
#include <utils.h>
#include <stdio.h>

static int init_real(bgav_stream_t * s);
static int decode_real(bgav_stream_t * s, gavl_audio_frame_t * f, int num_samples);
static void close_real(bgav_stream_t * s);
static void resync_real(bgav_stream_t * s);
// static void skip_real(bgav_stream_t * s,int);

typedef struct
  {
  const char * dll_name;
  const char * format_name;
  bgav_audio_decoder_t decoder;
  } codec_info_t;

static codec_info_t real_codecs[] =
  {
    {
      dll_name: "dnet.so.6.0",
      format_name: "Real Audio dnet",
      decoder:
      {
        name:   "Real audio dnet DLL decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('d', 'n', 'e', 't'), 0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
    },
    {
      dll_name: "sipr.so.6.0",
      format_name: "Real Audio sipr",
      decoder:
      {
        name:   "Real audio sipr DLL decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('s', 'i', 'p', 'r'), 0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
    },
    {
      dll_name: "cook.so.6.0",
      format_name: "Real Audio cook",
      decoder:
      {
        name:   "Real audio cook DLL decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('c', 'o', 'o', 'k'), 0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
    },
#if 0
    {
      dll_name: "14_4.so.6.0",
      format_name: "Real Audio 14.4",
      decoder:
      {
        name:   "Real audio 14.4 DLL decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('1', '4', '_', '4'),
                                BGAV_MK_FOURCC('l', 'p', 'c', 'J'),
                                0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
      
    },
#endif
#if 0
    {
      dll_name: "28_8.so.6.0",
      format_name: "Real Audio 28.8 DLL decoder",
      decoder:
      {
        name:   "Real audio 28.8 decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('2', '8', '_', '8'), 0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
      
    },
#endif
    {
      dll_name: "atrc.so.6.0",
      format_name: "Real Audio atrc",
      decoder:
      {
        name:   "Real audio atrc DLL decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('a', 't', 'r', 'c'),
                                0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
      
    },
  };

int bgav_init_audio_decoders_real()
  {
  int ret = 1;
  struct stat stat_buf;
  char test_filename[PATH_MAX];
  int i;

  //  fprintf(stderr, "bgav_init_audio_decoders_real: %s\n", bgav_dll_path_real); 
  for(i = 0; i < sizeof(real_codecs) / sizeof(real_codecs[0]); i++)
    {
    sprintf(test_filename, "%s/%s", bgav_dll_path_real, real_codecs[i].dll_name);
    if(!stat(test_filename, &stat_buf))
      bgav_audio_decoder_register(&real_codecs[i].decoder);
    else
      {
      fprintf(stderr, "Cannot find file %s, disabling %s\n",
              test_filename, real_codecs[i].decoder.name);
      ret = 0;
      }
    }
  return ret;
  }

typedef struct /*__attribute__((__packed__))*/ {
    int samplerate;
    short bits;
    short channels;
    short quality;
    /* 2bytes padding here, by gcc */
    int bits_per_frame;
    int packetsize;
    int extradata_len;
    void* extradata;
} ra_init_t;

#if 0
static void dump_init_data(ra_init_t * i)
  {
  fprintf(stderr, "*** INIT DATA ******\n");
  fprintf(stderr, "samplerate:     %d\n", i->samplerate);
  fprintf(stderr, "bits:           %d\n", i->bits);
  fprintf(stderr, "channels:       %d\n", i->channels);
  fprintf(stderr, "quality:        %d\n", i->quality);
  fprintf(stderr, "bits_per_frame: %d\n", i->bits_per_frame);
  fprintf(stderr, "packetsize:     %d\n", i->packetsize);
  fprintf(stderr, "extradata_len:  %d\n", i->extradata_len);
  bgav_hexdump(i->extradata, i->extradata_len, 16);
  fprintf(stderr, "*** END INIT DATA **\n");
  }
#endif

typedef struct
  {
  unsigned long (*raCloseCodec)(void*);
  unsigned long (*raDecode)(void*, char*,unsigned long,char*,unsigned int*,long);
  unsigned long (*raFlush)(unsigned long,unsigned long,unsigned long);
  unsigned long (*raFreeDecoder)(void*);
  void*         (*raGetFlavorProperty)(void*,unsigned long,unsigned long,int*);
  //unsigned long (*raGetNumberOfFlavors2)(void);
  unsigned long (*raInitDecoder)(void*, void*);
  unsigned long (*raOpenCodec)(void*);
  unsigned long (*raOpenCodec2)(void*, void*);
  unsigned long (*raSetFlavor)(void*,unsigned long);
  void  (*raSetDLLAccessPath)(char*);
  void  (*raSetPwd)(char*,char*);

  void * module;
  void * real_handle;

  uint8_t * read_buffer;
  uint8_t * read_buffer_ptr;
  int read_buffer_alloc;
  int read_buffer_size;

  //  int16_t * sample_buffer;
  //  int16_t * sample_buffer_ptr;
  //  int sample_buffer_size;

  gavl_audio_frame_t * frame;
  int last_frame_size;
    } real_priv_t;


static int init_real(bgav_stream_t * s)
  {
  ra_init_t init_data;
  char codec_filename[PATH_MAX];
  real_priv_t * priv;
  int i;
  char * path;
  int len;
  void * prop;
  codec_info_t * info = (codec_info_t*)0;
    
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;
  
  for(i = 0; i < sizeof(real_codecs) / sizeof(real_codecs[0]); i++)
    {
    if(&(real_codecs[i].decoder) == s->data.audio.decoder->decoder)
      {
      info = &real_codecs[i];
      break;
      }
    }

  if(!info)
    return 0;
  
  sprintf(codec_filename, "%s/%s", bgav_dll_path_real, info->dll_name);
  
  
  /* Try to dlopen it */

  if(!(priv->module = dlopen(codec_filename, RTLD_NOW)))
    {
    fprintf(stderr, "Could not open DLL %s %s\n", codec_filename, dlerror());
    return 0;
    }

  //  fprintf(stderr, "dlopen: %p\n", priv->module);

  /* Get the symbols */

  priv->raCloseCodec = dlsym(priv->module, "RACloseCodec");
  priv->raDecode = dlsym(priv->module, "RADecode");
  priv->raFlush = dlsym(priv->module, "RAFlush");
  priv->raFreeDecoder = dlsym(priv->module, "RAFreeDecoder");
  priv->raGetFlavorProperty = dlsym(priv->module, "RAGetFlavorProperty");
  priv->raOpenCodec = dlsym(priv->module, "RAOpenCodec");
  priv->raOpenCodec2 = dlsym(priv->module, "RAOpenCodec2");
  priv->raInitDecoder = dlsym(priv->module, "RAInitDecoder");
  priv->raSetFlavor = dlsym(priv->module, "RASetFlavor");
  priv->raSetDLLAccessPath = dlsym(priv->module, "SetDLLAccessPath");
  priv->raSetPwd = dlsym(priv->module, "RASetPwd"); // optional, used by SIPR

  if(!(priv->raCloseCodec &&
       priv->raDecode &&
       priv->raFreeDecoder &&
       priv->raGetFlavorProperty &&
       (priv->raOpenCodec||priv->raOpenCodec2) &&
       priv->raSetFlavor &&
       priv->raInitDecoder))
    return 0;

  path= bgav_sprintf("DT_Codecs=%s", bgav_dll_path_real);
  if(path[strlen(path)-1]!='/')
    path = bgav_strncat(path, "/", NULL);

  /* Append one zero byte */

  path = realloc(path, strlen(path)+2);
  path[strlen(path)+1] = '\0';
  
  /* Set the codec path */

  if(priv->raSetDLLAccessPath)
    {
    // used by 'SIPR'
    priv->raSetDLLAccessPath(path);
    }

  if(priv->raOpenCodec2)
    {
    if(priv->raOpenCodec2(&(priv->real_handle),&(path[10])))
      {
      fprintf(stderr, "raOpenCodec2 failed\n");
      return 0;
      }
    }
  else if(priv->raOpenCodec(priv->real_handle))
    {
    fprintf(stderr, "raOpenCodec2 failed\n");
    return 0;
    }

  free(path);
  
  init_data.samplerate = s->data.audio.format.samplerate;
  init_data.bits       = s->data.audio.bits_per_sample;
  init_data.channels   = s->data.audio.format.num_channels;
  init_data.quality    = 100;
    /* 2bytes padding here, by gcc */
  init_data.bits_per_frame = ((short*)(s->ext_data))[0];
  init_data.packetsize  =    ((short*)(s->ext_data))[3];

  init_data.extradata_len =  ((short*)(s->ext_data))[4];
  init_data.extradata =       s->ext_data+10;

  //  dump_init_data(&init_data);

  if(priv->raInitDecoder(priv->real_handle,&init_data))
    {
    fprintf(stderr, "raInitDecoder failed\n");
    return 0;
    }
  if(priv->raSetPwd)
    {
    priv->raSetPwd(priv->real_handle,"Ardubancel Quazanga");
    }
  
  if(priv->raSetFlavor(priv->real_handle,((short*)(s->ext_data))[2]))
    {
    fprintf(stderr, "raSetFlavor failed\n");
    return 0;
    }

  prop = priv->raGetFlavorProperty(priv->real_handle, ((short*)(s->ext_data))[2], 0, &len);
  
  if(prop)
    s->description = bgav_sprintf("%s (Flavor: %s)", info->format_name, prop);
  else
    s->description = bgav_sprintf("%s (Flavor: %s)", info->format_name);
  //  fprintf(stderr, "FlavorProperty: %s\n", prop);
  
  prop = priv->raGetFlavorProperty(priv->real_handle, ((short*)(s->ext_data))[2], 1, &len);
  
  /* Allocate sample buffer and set audio format */
    
  
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.sample_format   = GAVL_SAMPLE_S16;
  gavl_set_channel_setup(&(s->data.audio.format));

  s->data.audio.format.samples_per_frame = 10240;
  priv->frame = gavl_audio_frame_create(&(s->data.audio.format));
  s->data.audio.format.samples_per_frame = 1024;
  
  return 1;
  }

static unsigned char sipr_swaps[38][2]={
    {0,63},{1,22},{2,44},{3,90},{5,81},{7,31},{8,86},{9,58},{10,36},{12,68},
    {13,39},{14,73},{15,53},{16,69},{17,57},{19,88},{20,34},{21,71},{24,46},
    {25,94},{26,54},{28,75},{29,50},{32,70},{33,92},{35,74},{38,85},{40,56},
    {42,87},{43,65},{45,59},{48,79},{49,93},{51,89},{55,95},{61,76},{67,83},
    {77,80} };

static int fill_buffer(bgav_stream_t * s)
  {
  real_priv_t * priv;
  bgav_packet_t * p;
  int sps=((short*)(s->ext_data))[0];
  int w=s->data.audio.block_align; // 5
  int h=((short*)(s->ext_data))[1];
  int cfs=((short*)(s->ext_data))[3];
  priv = (real_priv_t*)(s->data.audio.decoder->priv);

  p = bgav_demuxer_get_packet_read(s->demuxer, s);

  if(!p)
    {
    //    fprintf(stderr, "Got no packet\n");
    return 0;
    }
  if(p->data_size > priv->read_buffer_alloc)
    {
    priv->read_buffer_alloc = p->data_size;
    priv->read_buffer = realloc(priv->read_buffer, priv->read_buffer_alloc);
    }
  priv->read_buffer_size = p->data_size;
  priv->read_buffer_ptr  = priv->read_buffer;

  //  fprintf(stderr, "data_size: %d\n", p->data_size);
  // hexdump(p->data, p->data_size, 7);
  
  /* Now, read and descramble the stuff */

  if((s->fourcc == BGAV_MK_FOURCC('1','4','_','4')) ||
     (s->fourcc == BGAV_MK_FOURCC('d','n','e','t')))
    {
    memcpy(priv->read_buffer, p->data, p->data_size);
    //    fprintf(stderr, "COPY 14.4\n");
    }
  else if(s->fourcc == BGAV_MK_FOURCC('2','8','_','8'))
    {
    int i,j, idx = 0;
    //    fprintf(stderr, "COPY 28.8\n");
    for (j = 0; j < h; j++)
      {
      for (i = 0; i < h/2; i++)
        {
        memcpy(priv->read_buffer+i*2*w+j*cfs, p->data + idx * cfs, cfs);
        idx++;
        }
      }
    }
  else if(!sps)
    {
    // 'sipr' way
    int j,n;
    int bs=h*w*2/96; // nibbles per subpacket
    unsigned char *ptr;
    //    fprintf(stderr, "COPY sipr\n");

    memcpy(priv->read_buffer, p->data, p->data_size);
    ptr = priv->read_buffer;
    for(n=0;n<38;n++)
      {
      int i=bs*sipr_swaps[n][0];
      int o=bs*sipr_swaps[n][1];
      // swap nibbles of block 'i' with 'o'      TODO: optimize
      for(j=0;j<bs;j++)
        {
        int x=(i&1) ? (ptr[(i>>1)]>>4) : (ptr[(i>>1)]&15);
        int y=(o&1) ? (ptr[(o>>1)]>>4) : (ptr[(o>>1)]&15);
        if(o&1) ptr[(o>>1)]=(ptr[(o>>1)]&0x0F)|(x<<4);
        else  ptr[(o>>1)]=(ptr[(o>>1)]&0xF0)|x;
        if(i&1) ptr[(i>>1)]=(ptr[(i>>1)]&0x0F)|(y<<4);
        else  ptr[(i>>1)]=(ptr[(i>>1)]&0xF0)|y;
        ++i;++o;
        }
      }
    }
  else
    {
    // 'cook' way
    int x,y,idx=0;
    fprintf(stderr, "COPY cook %d %d %d %p\n", w, h, sps, priv->read_buffer);
    w/=sps;
    for(y=0;y<h;y++)
      for(x=0;x<w;x++)
        {
        memcpy(priv->read_buffer+sps*(h*x+((h+1)/2)*(y&1)+(y>>1)),
               p->data + idx * sps, sps);
        idx++;
        }
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static int decode_frame(bgav_stream_t * s)
  {
  unsigned int len;

  real_priv_t * priv;
  priv = (real_priv_t*)(s->data.audio.decoder->priv);
  if(!priv->read_buffer_size)
    {
    if(!fill_buffer(s))
      {
      //      fprintf(stderr, "No more data\n");
      return 0;
      }
    //    else
    //      fprintf(stderr, "Read data: %d\n", priv->read_buffer_size);
    }
  /* Call the decoder */

  if(priv->raDecode(priv->real_handle, (char*)priv->read_buffer_ptr,
                    s->data.audio.block_align,
                    (char*)priv->frame->samples.s_8, &len, -1))
    {
    fprintf(stderr, "raDecode failed\n");
    }
  //  fprintf(stderr, "Used %d bytes, decoded %d samples\n",
  //          s->data.audio.block_align, len / (2 * s->data.audio.format.num_channels));
  priv->read_buffer_ptr += s->data.audio.block_align;
  priv->read_buffer_size -= s->data.audio.block_align;
  
  priv->frame->valid_samples = len / (2 * s->data.audio.format.num_channels);
  priv->last_frame_size = priv->frame->valid_samples;
  return 1;
  }

static int decode_real(bgav_stream_t * s, gavl_audio_frame_t * f, int num_samples)
  {
  int samples_copied;
  int samples_decoded = 0;
  real_priv_t * priv;
  priv = (real_priv_t*)(s->data.audio.decoder->priv);
  
  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      //      fprintf(stderr, "decode frame...");
      if(!decode_frame(s))
        {
        if(f)
          f->valid_samples = samples_decoded;
        //        fprintf(stderr, "Decode frame failed %d\n", samples_decoded);
        return samples_decoded;
        }
      //      fprintf(stderr, "done\n");
      }
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           f,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->last_frame_size - priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    //    fprintf(stderr, "Samples decoded %d\n", samples_decoded);
    }
  if(f)
    f->valid_samples = samples_decoded;
  return samples_decoded;


  }
#if 0

static void skip_real(bgav_stream_t * s, int num_samples)
  {
  real_priv_t * p = (real_priv_t*)s->data.audio.decoder->priv;
  if(p->sample_buffer_size)
    {
    
    }
  }

typedef struct
  {
  unsigned long (*raCloseCodec)(void*);
  unsigned long (*raDecode)(void*, char*,unsigned long,char*,unsigned int*,long);
  unsigned long (*raFlush)(unsigned long,unsigned long,unsigned long);
  unsigned long (*raFreeDecoder)(void*);
  void*         (*raGetFlavorProperty)(void*,unsigned long,unsigned long,int*);
  //unsigned long (*raGetNumberOfFlavors2)(void);
  unsigned long (*raInitDecoder)(void*, void*);
  unsigned long (*raOpenCodec)(void*);
  unsigned long (*raOpenCodec2)(void*, void*);
  unsigned long (*raSetFlavor)(void*,unsigned long);
  void  (*raSetDLLAccessPath)(char*);
  void  (*raSetPwd)(char*,char*);

  void * module;
  void * real_handle;

  uint8_t * read_buffer;
  uint8_t * read_buffer_ptr;
  int read_buffer_alloc;
  int read_buffer_size;

  int16_t * sample_buffer;
  int16_t * sample_buffer_ptr;
  int sample_buffer_size;
  
  } real_priv_t;
#endif

static void close_real(bgav_stream_t * s)
  {
  real_priv_t * p = (real_priv_t*)s->data.audio.decoder->priv;

  if(p->frame)
    gavl_audio_frame_destroy(p->frame);
  
  if(p->read_buffer)
    free(p->read_buffer);
#if 1
  if(p->raFreeDecoder)
    p->raFreeDecoder(p->real_handle);
  if(p->raCloseCodec)
    p->raCloseCodec(p->real_handle);
#endif

  //  fprintf(stderr, "dlclose: %p\n", p->module);
  
  //  dlclose(p->module);
  free(p);
  }

static void resync_real(bgav_stream_t * s)
  {
  real_priv_t * p = (real_priv_t*)s->data.audio.decoder->priv;
  p->frame->valid_samples = 0;
  p->read_buffer_size = 0;

  //  fprintf(stderr, "clear realaud\n");
  }

