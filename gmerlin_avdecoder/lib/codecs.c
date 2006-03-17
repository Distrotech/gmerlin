/*****************************************************************
 
  codecs.c
 
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

#include <avdec_private.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <codecs.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <utils.h>

// #define ENABLE_DEBUG

static void codecs_lock();
static void codecs_unlock();


#if (HAVE_REALDLL || HAVE_XADLL || HAVE_W32DLL)
static const char * find_directory(char ** dirs, const char * env_name)
  {
  struct stat st;
  const char * env_dir;
  int i;
  i = 0;
  while(dirs[i])
    {
    if(!stat(dirs[i], &st) && S_ISDIR(st.st_mode))
      return dirs[i];
    else
      i++;
    }
  
  if((env_dir = getenv(env_name)) && !stat(env_dir, &st) && S_ISDIR(st.st_mode))
    return env_dir;
    
  return (char*)0;
  }
#endif

#ifdef HAVE_REALDLL
static const char * env_name_real = "GMERLIN_AVDEC_CODEC_PATH_REAL";

static char * real_dirs[] =
  {
    "/usr/lib/codecs",
    "/usr/local/lob/codecs",
    "/usr/lib/codecdlls",
    "/usr/lib/win32",
    "/usr/local/lib/win32",
    (char*)0
  };

static void bgav_set_dll_path_real()
  {
  if(bgav_dll_path_real)
    {
    free(bgav_dll_path_real);
    }
  bgav_dll_path_real = bgav_strndup(find_directory(real_dirs, env_name_real), NULL);
  }
#endif

#ifdef HAVE_XADLL
static const char * env_name_xanim =  "GMERLIN_AVDEC_CODEC_PATH_XANIM";

static char * xanim_dirs[] =
  {
    "/usr/lib/codecs",
    "/usr/local/lob/codecs",
    "/usr/lib/codecdlls",
    "/usr/lib/win32",
    "/usr/local/lib/win32",
    (char*)0
  };

static void bgav_set_dll_path_xanim()
  {
  if(bgav_dll_path_xanim)
    {
    free(bgav_dll_path_xanim);
    }
  bgav_dll_path_xanim = bgav_strndup(find_directory(xanim_dirs, env_name_xanim), NULL);
  }
#endif

#ifdef HAVE_W32DLL
static const char * env_name_win32 = "GMERLIN_AVDEC_CODEC_PATH_WIN32";

static char * win32_dirs[] =
  {
    "/usr/lib/codecs",
    "/usr/local/lob/codecs",
    "/usr/lib/codecdlls",
    "/usr/lib/win32",
    "/usr/local/lib/win32",
    (char*)0
  };
  
static int win_path_needs_delete = 0;

static void bgav_set_dll_path_win32()
  {
  if(win_path_needs_delete)
    {
    free(win32_def_path);
    }
  win32_def_path = bgav_strndup(find_directory(win32_dirs, env_name_win32), NULL);
  if(win32_def_path)
    win_path_needs_delete = 1;
  }
#endif

/* Simple codec registry */

/*
 *  Since we link all codecs statically,
 *  this stays quite simple
 */

static bgav_audio_decoder_t * audio_decoders = (bgav_audio_decoder_t*)0;
static bgav_video_decoder_t * video_decoders = (bgav_video_decoder_t*)0;
static bgav_subtitle_overlay_decoder_t * subtitle_overlay_decoders =
  (bgav_subtitle_overlay_decoder_t*)0;

static int codecs_initialized = 0;

static int num_audio_codecs = 0;
static int num_video_codecs = 0;
static int num_subtitle_overlay_codecs = 0;

static pthread_mutex_t codec_mutex;
static int mutex_initialized = 0;


static void codecs_lock()
  {
  //  fprintf(stderr, "codecs_lock\n");
  
  if(!mutex_initialized)
    {
    pthread_mutex_init(&(codec_mutex),(pthread_mutexattr_t *)0);
    mutex_initialized = 1;
    }
  pthread_mutex_lock(&(codec_mutex));
  }

static void codecs_unlock()
  {
  //  fprintf(stderr, "codecs_unlock\n");
  pthread_mutex_unlock(&(codec_mutex));
  }


void bgav_codecs_dump()
  {
  bgav_audio_decoder_t * ad;
  bgav_video_decoder_t * vd;
  bgav_subtitle_overlay_decoder_t * sod;
  int i;
  bgav_codecs_init();
  
  /* Print */
  ad = audio_decoders;

  fprintf(stderr, "<h2>Audio codecs</h2>\n");

  fprintf(stderr, "<ul>\n");
  for(i = 0; i < num_audio_codecs; i++)
    {
    fprintf(stderr, "<li>%s\n", ad->name);
    ad = ad->next;
    }
  fprintf(stderr, "</ul>\n");
  
  fprintf(stderr, "<h2>Video codecs</h2>\n");
  fprintf(stderr, "<ul>\n");
  vd = video_decoders;
  for(i = 0; i < num_video_codecs; i++)
    {
    fprintf(stderr, "<li>%s\n", vd->name);
    vd = vd->next;
    }
  fprintf(stderr, "</ul>\n");

  fprintf(stderr, "<h2>Graphical subtitle codecs</h2>\n");
  fprintf(stderr, "<ul>\n");
  sod = subtitle_overlay_decoders;
  for(i = 0; i < num_subtitle_overlay_codecs; i++)
    {
    fprintf(stderr, "<li>%s\n", sod->name);
    sod = sod->next;
    }
  fprintf(stderr, "</ul>\n");

  }

/*
 *  Print some verbose error messages so users will know
 *  how to set up their systems
 */

static void print_dirs(char ** dirs)
  {
  int i = 0;
  
  fprintf(stderr, "\n");
  while(dirs[i])
    {
    fprintf(stderr, "%s\n", dirs[i]);
    i++;
    }
  fprintf(stderr, "\n");
  }

static void print_error_nopath(const char * name, const char * env_name, char ** dirs)
  {
  
  fprintf(stderr, "Didn't find path for %s\n", name);
  fprintf(stderr, "Install it in one of the following directories:\n");
  print_dirs(dirs);
  
  fprintf(stderr, "or set the environment variable %s\n\n", env_name);
  }

static void print_error_nofile(const char * name, const char * env_name, char ** dirs)
  {
  fprintf(stderr, "Didn't find some %s\n", name);
  fprintf(stderr, "Install them in one of the following directories:\n");
  print_dirs(dirs);
  fprintf(stderr, "Or set the the environment variable %s\n\n",
          env_name);
  }

void bgav_codecs_init()
  {
  //  fprintf(stderr, "bgav_codecs_init()\n");
  codecs_lock();
  if(codecs_initialized)
    {
    codecs_unlock();
    return;
    }
  //  fprintf(stderr, "bgav_codecs_init() 1\n");
  codecs_initialized = 1;
  
#ifdef HAVE_LIBAVCODEC
  bgav_init_audio_decoders_ffmpeg();
  bgav_init_video_decoders_ffmpeg();
#endif
#ifdef HAVE_VORBIS
  bgav_init_audio_decoders_vorbis();
#endif

#ifdef HAVE_LIBA52
  bgav_init_audio_decoders_a52();
#endif

#ifdef HAVE_MAD
  bgav_init_audio_decoders_mad();
#endif

#ifdef HAVE_LIBPNG
  bgav_init_video_decoders_png();
#endif

#ifdef HAVE_LIBTIFF
  bgav_init_video_decoders_tiff();
#endif

#ifdef HAVE_THEORA
  bgav_init_video_decoders_theora();
#endif

#ifdef HAVE_SPEEX
  bgav_init_audio_decoders_speex();
#endif

#ifdef HAVE_FAAD2
  bgav_init_audio_decoders_faad2();
#endif

#ifdef HAVE_FLAC
  bgav_init_audio_decoders_flac();
#endif

  
#ifdef HAVE_LIBMPEG2
  bgav_init_video_decoders_libmpeg2();
#endif
  
#ifdef HAVE_XADLL

  bgav_set_dll_path_xanim();

  if(!bgav_dll_path_xanim)
    print_error_nopath("Xanim dlls", env_name_xanim, xanim_dirs);
  else
    {
    if(!bgav_init_video_decoders_xadll())
      {
      print_error_nofile("Xanim dlls", env_name_xanim, xanim_dirs);
      }
    }
#endif

#ifdef HAVE_REALDLL
  
  bgav_set_dll_path_real();

  if(!bgav_dll_path_real)
    print_error_nopath("Real DLLs", env_name_real, real_dirs);
  else
    {
    if(!bgav_init_video_decoders_real())
      {
      print_error_nofile("Real video DLLs", env_name_real, real_dirs);
      }
    if(!bgav_init_audio_decoders_real())
      {
      print_error_nofile("Real audio DLLs", env_name_real, real_dirs);
      }
    }
#endif

#ifdef HAVE_W32DLL
  
  bgav_set_dll_path_win32();

  if(!win_path_needs_delete)
    {
    print_error_nopath("Win32 DLLs", env_name_win32, win32_dirs);
    }
  else
    {
    if(!bgav_init_video_decoders_win32())
      {
      print_error_nofile("Win32 video DLLs", env_name_win32, win32_dirs);
      }
    if(!bgav_init_audio_decoders_win32())
      {
      print_error_nofile("Win32 audio DLLs", env_name_win32, win32_dirs);
      }
    if(!bgav_init_audio_decoders_qtwin32())
      {
      print_error_nofile("Win32 Quicktime audio DLLs", env_name_win32, win32_dirs);
      }
    }
  
#endif
  
  bgav_init_audio_decoders_gavl();
  bgav_init_audio_decoders_pcm();
  bgav_init_audio_decoders_gsm();
  bgav_init_video_decoders_aviraw();
  bgav_init_video_decoders_qtraw();
  bgav_init_video_decoders_yuv();
  bgav_init_video_decoders_tga();
  bgav_init_video_decoders_rtjpeg();
  bgav_init_video_decoders_gavl();

  bgav_init_subtitle_overlay_decoders_dvd();

#if 0  
  fprintf(stderr, "BGAV Codecs initialized: A: %d V: %d\n",
          num_audio_codecs, num_video_codecs);
#endif
  codecs_unlock();
  
  }

void bgav_audio_decoder_register(bgav_audio_decoder_t * dec)
  {
  bgav_audio_decoder_t * before;
  if(!audio_decoders)
    audio_decoders = dec;
  else
    {
    before = audio_decoders;
    while(before->next)
      before = before->next;
    before->next = dec;
    }
  dec->next = (bgav_audio_decoder_t*)0;
  num_audio_codecs++;
  }

void bgav_video_decoder_register(bgav_video_decoder_t * dec)
  {
  bgav_video_decoder_t * before;
  if(!video_decoders)
    video_decoders = dec;
  else
    {
    before = video_decoders;
    while(before->next)
      before = before->next;
    before->next = dec;
    }
  dec->next = (bgav_video_decoder_t*)0;
  num_video_codecs++;
  }

void bgav_subtitle_overlay_decoder_register(bgav_subtitle_overlay_decoder_t * dec)
  {
  bgav_subtitle_overlay_decoder_t * before;
  if(!subtitle_overlay_decoders)
    subtitle_overlay_decoders = dec;
  else
    {
    before = subtitle_overlay_decoders;
    while(before->next)
      before = before->next;
    before->next = dec;
    }
  dec->next = (bgav_subtitle_overlay_decoder_t*)0;
  num_subtitle_overlay_codecs++;
  
  }

bgav_audio_decoder_t * bgav_find_audio_decoder(bgav_stream_t * s)
  {
  bgav_audio_decoder_t * cur;
  int i;
  codecs_lock();
  cur = audio_decoders;
  //  if(!codecs_initialized)
  //    bgav_codecs_init();

#ifdef ENABLE_DEBUG
  fprintf(stderr, "Seeking audio codec ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
#endif
  
  while(cur)
    {
    i = 0;
    //    fprintf(stderr, "Trying %s\n", cur->name);
    while(cur->fourccs[i])
      {
      if(cur->fourccs[i] == s->fourcc)
        {
        codecs_unlock();
#ifdef ENABLE_DEBUG
        fprintf(stderr, "Found %s\n", cur->name);
#endif
        return cur;
        }
      else
        i++;
      }
    cur = cur->next;
    }
  codecs_unlock();
  return (bgav_audio_decoder_t*)0;
  }

bgav_video_decoder_t * bgav_find_video_decoder(bgav_stream_t * s)
  {
  bgav_video_decoder_t * cur;
  int i;
  codecs_lock();

  //  if(!codecs_initialized)
  //    bgav_codecs_init();
  
  cur = video_decoders;

#ifdef ENABLE_DEBUG
  fprintf(stderr, "Seeking video codec ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
#endif
  while(cur)
    {
    i = 0;
    while(cur->fourccs[i])
      {
      if(cur->fourccs[i] == s->fourcc)
        {
        codecs_unlock();
#ifdef ENABLE_DEBUG
        fprintf(stderr, "Found %s\n", cur->name);
#endif
        return cur;
        }
      else
        i++;
      }
    cur = cur->next;
    }
  codecs_unlock();
  return (bgav_video_decoder_t*)0;
  }

bgav_subtitle_overlay_decoder_t * bgav_find_subtitle_overlay_decoder(bgav_stream_t * s)
  {
  bgav_subtitle_overlay_decoder_t * cur;
  int i;
  codecs_lock();

  //  if(!codecs_initialized)
  //    bgav_codecs_init();
  
  cur = subtitle_overlay_decoders;

#ifdef ENABLE_DEBUG
  fprintf(stderr, "Seeking subtitle overlay codec ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
#endif
  while(cur)
    {
    i = 0;
    while(cur->fourccs[i])
      {
      if(cur->fourccs[i] == s->fourcc)
        {
        codecs_unlock();
#ifdef ENABLE_DEBUG
        fprintf(stderr, "Found %s\n", cur->name);
#endif
        return cur;
        }
      else
        i++;
      }
    cur = cur->next;
    }
  codecs_unlock();
  return (bgav_subtitle_overlay_decoder_t*)0;
  }


/* Free codec strings */

/***************************************************************
 * This will hopefully make the destruction for dynamic loading
 * (Trick comes from a 1995 version of the ELF Howto, so it
 * should work everywhere now)
 ***************************************************************/

#if defined(__GNUC__) && defined(__ELF__)
static void __cleanup() __attribute__ ((destructor));
 
static void __cleanup()
  {
  //  fprintf(stderr, "Freeing codec paths\n");

  if(mutex_initialized)
    pthread_mutex_destroy(&codec_mutex);
    
#ifdef HAVE_W32DLL
  if(win_path_needs_delete)
    {
    free(win32_def_path);
    }
#endif
#ifdef HAVE_REALDLL
  if(bgav_dll_path_real)
    {
    free(bgav_dll_path_real);
    }
#endif  
#ifdef HAVE_XADLL
  if(bgav_dll_path_xanim)
    {
    free(bgav_dll_path_xanim);
    }
#endif
  }

#endif
