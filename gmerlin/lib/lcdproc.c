#include <config.h>
#include <parameter.h>
#include <player.h>
#include <lcdproc.h>
#include <tcp.h>

#include <string.h>
#include <unistd.h>
#include <utils.h>

static const char * formats_name      = "formats";
static const char * name_time_name    = "name_time";
static const char * descriptions_name = "descriptions";

static const char * audio_format_name        = "audio_format";
static const char * video_format_name        = "video_format";

static const char * audio_description_name   = "audio_description";
static const char * video_description_name   = "video_description";

static const char * name_name                = "name";
static const char * time_name                = "time";

struct bg_lcdproc_s
  {
  int fd;

  int enable_lcdproc;
  int display_name_time;
  int display_formats;
  int display_descriptions;
  
  char * answer;
  int answer_alloc;
    
  /* Values used for the current connection */
  char * hostname;
  int port;

  /* Config values */
  char * hostname_cfg;
  int port_cfg;

  /* Connection to the player */

  bg_msg_queue_t * queue;

  pthread_mutex_t config_mutex;

  /* screens */

  int have_name_time;
  int have_formats;
  int have_descriptions;
  
  /* Thread handling */
  pthread_t thread;

  pthread_mutex_t state_mutex;
  int is_running;
  int do_stop;

  bg_player_t * player;
  };

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "enable_lcdproc",
      long_name:   "Enable LCDproc",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name: "hostname",
      long_name: "Hostname",
      type: BG_PARAMETER_STRING,
      val_default: { val_str: "localhost" },
    },
    {
      name:        "port",
      long_name:   "Port",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 1024 },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 13666 },
    },
    {
      name: "display_name_time",
      long_name: "Display track name and time",
      type: BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name: "display_formats",
      long_name: "Display audio/video formats",
      type: BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name: "display_descriptions",
      long_name: "Display audio/video descriptions",
      type: BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    { /* End of parameters */ }
  };


bg_lcdproc_t * bg_lcdproc_create(bg_player_t * player)
  {
  bg_lcdproc_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->fd = -1;
  ret->queue = bg_msg_queue_create();
  pthread_mutex_init(&(ret->config_mutex), (pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->state_mutex), (pthread_mutexattr_t *)0);
  ret->player = player;
  return ret;
  }

#define FREE(p) if(l->p) free(l->p);

void bg_lcdproc_destroy(bg_lcdproc_t* l)
  {
  FREE(answer);
  FREE(hostname);
  FREE(hostname_cfg);
  bg_msg_queue_destroy(l->queue);

  pthread_mutex_destroy(&(l->config_mutex));
  pthread_mutex_destroy(&(l->state_mutex));
  free(l);
  }

int send_command(bg_lcdproc_t * l, char * command)
  {
  char * error_msg = (char*)0;

  if(!bg_tcp_send(l->fd, command, strlen(command), &error_msg))
    return 0;

  while(1)
    {
    if(!bg_tcp_read_line(l->fd, &(l->answer), &(l->answer_alloc), 500))
      return 0;
    if(!strncmp(l->answer, "success", 7))
      break;
    else if(!strncmp(l->answer, "huh", 3))
      {
      fprintf(stderr, "Got server error: %s\n", l->answer);
      return 0;
      }
    }
  
  return 1;
  }

/* Here comes the real working stuff */

static int do_connect(bg_lcdproc_t* l)
  {
  char * error_msg = (char*)0;

  //  fprintf(stderr, "Connecting to %s:%d...", l->hostname_cfg, l->port_cfg);
  l->fd = bg_tcp_connect(l->hostname_cfg, l->port_cfg, 500,
                         &error_msg);
  if(l->fd < 0)
    {
    //    fprintf(stderr, "Failed\n");
    goto fail;
    }
  //  fprintf(stderr, "Done\n");
  /* Send hello and get answer */
  if(!bg_tcp_send(l->fd, "hello\n", 6, &error_msg))
    goto fail;

  if(!bg_tcp_read_line(l->fd, &(l->answer), &(l->answer_alloc), 500))
    goto fail;

  //  fprintf(stderr, "Got answer: %s\n", l->answer);

  if(strncmp(l->answer, "connect LCDproc", 15))
    {
    error_msg = bg_sprintf("Invalid answer: %s\n", l->answer);
    goto fail;
    }

  /* Set client attributes */

  if(!send_command(l, "client_set name {gmerlin}\n"))
    goto fail;
  
  return 1;
  fail:
  if(error_msg)
    {
    //    fprintf(stderr, "Connecting failed: %s\n", error_msg);
    free(error_msg);
    }
  return 0;
  }

static int set_name(bg_lcdproc_t * l, const char * name)
  {
  char * command;
  command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {%s *** }\n",
                       name_time_name, name_name, name);
  if(!send_command(l, command))
    goto fail;
  free(command); 
  return 1;
  fail:
  free(command); 
  return 0;
  }

static int set_time(bg_lcdproc_t * l, gavl_time_t time)
  {
  char * command;
  char buffer[16]; /* MUST be larger than GAVL_TIME_STRING_LEN */
  
  if(time == GAVL_TIME_UNDEFINED)
    {
    gethostname(buffer, 16);
    
    command = bg_sprintf("widget_set %s %s 1 1 {%s}\n",
                         name_time_name, time_name, buffer);
    if(!send_command(l, command))
      goto fail;
    }
  else
    {
    gavl_time_prettyprint(time, buffer);
    
    command = bg_sprintf("widget_set %s %s 1 1 {Time: %10s}\n",
                         name_time_name, time_name, buffer);
    if(!send_command(l, command))
      goto fail;
    }
  
  free(command);
  return 1;
  fail:
  free(command);  
  return 0;
  }

static int create_name_time(bg_lcdproc_t * l)
  {
  //  char time_buf[GAVL_TIME_STRING_LEN];
    
  char * command;
  command = bg_sprintf("screen_add %s\n", name_time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  command = bg_sprintf("screen_set %s heartbeat off\n", name_time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  
  /* Time display */
  
  command = bg_sprintf("widget_add %s %s string\n",
                       name_time_name, time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_time(l, GAVL_TIME_UNDEFINED);
  
  /* Name display */
  command = bg_sprintf("widget_add %s %s scroller\n",
                       name_time_name, name_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  set_name(l, PACKAGE"-"VERSION);
  l->have_name_time = 1;
  return 1;

  fail:

  free(command);
  return 0;
  }

static int destroy_name_time(bg_lcdproc_t * l)
  {
  char * command;

  command = bg_sprintf("screen_del %s\n", name_time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  l->have_name_time = 0;
  
  return 1;
  fail:
  free(command);
  return 0;

  }

static int set_audio_format(bg_lcdproc_t * l, gavl_audio_format_t * f)
  {
  char * command;

  if(!f)
    {
    command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio: none}\n",
                         formats_name, audio_format_name);
    }
  else
    {
    if(f->num_channels == 1)
      command = 
        bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio format: %d Hz Mono *** }\n", 
                  formats_name, audio_format_name, f->samplerate);
    else if((f->num_channels == 2) && 
            (f->channel_setup == GAVL_CHANNEL_STEREO))
      command =         bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio format: %d Hz Stereo *** }\n",
                  formats_name, audio_format_name, f->samplerate);
    else
      command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio format: %d Hz %d Channels *** }\n",
                         formats_name, audio_format_name, f->samplerate, f->num_channels);
    }

  
  if(!send_command(l, command))
    goto fail;
  free(command); 
  return 1;
  fail:
  free(command); 
  return 0;

  }

static int set_video_format(bg_lcdproc_t * l, gavl_video_format_t * f)
  {
  char * command;
  if(!f)
    {
    command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {Video: none}\n",
                         formats_name, video_format_name);
    }
  else
    {
    if(f->free_framerate)
      {
      command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {Video format: %dx%d *** }\n",
                           formats_name, video_format_name, f->image_width,
                           f->image_height);
      }
    else
      {
      command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {Video format: %dx%d %.2f fps *** }\n",
                           formats_name, video_format_name, f->image_width,
                           f->image_height, (float)(f->timescale)/
                           (float)(f->frame_duration));
      }
    }
  if(!send_command(l, command))
    goto fail;
  free(command); 
  return 1;
  fail:
  free(command); 
  return 0;
  }

static int create_formats(bg_lcdproc_t * l)
  {
  char * command;
  command = bg_sprintf("screen_add %s\n", formats_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  command = bg_sprintf("screen_set %s heartbeat off\n", formats_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  
  /* Audio format */

  command = bg_sprintf("widget_add %s %s scroller\n",
                       formats_name, audio_format_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_audio_format(l, NULL);
  
  /* Video format */
  
  command = bg_sprintf("widget_add %s %s scroller\n",
                       formats_name, video_format_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_video_format(l, NULL);
  l->have_formats = 1;
  return 1;

  fail:

  free(command);
  return 0;
  }

static int destroy_formats(bg_lcdproc_t * l)
  {
  char * command;
  
  command = bg_sprintf("screen_del %s\n", formats_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  l->have_formats = 0;
  return 1;
  fail:
  free(command);
  return 0;
  }

static int set_audio_description(bg_lcdproc_t * l, const char * desc)
  {
  char * command;

  if(!desc)
    {
    command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio: off}\n",
                         descriptions_name, audio_description_name);
    }
  else
    {
    command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio stream: %s *** }\n",
                         descriptions_name, audio_description_name, desc);
    }
  if(!send_command(l, command))
    goto fail;
  free(command); 
  return 1;
  fail:
  free(command); 
  return 0;

  }

static int set_video_description(bg_lcdproc_t * l, const char * desc)
  {
  char * command;
  if(!desc)
    {
    command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {Video: off}\n",
                         descriptions_name, video_description_name);
    }
  else
    {
    command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {Video stream: %s *** }\n",
                         descriptions_name, video_description_name, desc);
    }
  if(!send_command(l, command))
    goto fail;
  free(command); 
  return 1;
  fail:
  free(command); 
  return 0;
  }

static int create_descriptions(bg_lcdproc_t * l)
  {
  char * command;
  command = bg_sprintf("screen_add %s\n", descriptions_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  command = bg_sprintf("screen_set %s heartbeat off\n", descriptions_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  
  /* Audio description */

  command = bg_sprintf("widget_add %s %s scroller\n",
                       descriptions_name, audio_description_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_audio_description(l, NULL);
  
  /* Video format */
  
  command = bg_sprintf("widget_add %s %s scroller\n",
                       descriptions_name, video_description_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_video_description(l, NULL);
  l->have_descriptions = 1;
  
  return 1;

  fail:

  free(command);
  return 0;

  }

static int destroy_descriptions(bg_lcdproc_t * l)
  {
  char * command;
  
  command = bg_sprintf("screen_del %s\n", descriptions_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  l->have_descriptions = 1;
  return 1;
  fail:
  free(command);
  return 0;
  }

static void * thread_func(void * data)
  {
  int arg_i_1;
  char * arg_str_1;
  
  int id;
  gavl_time_t time;

  bg_msg_t * msg;
  bg_lcdproc_t * l = (bg_lcdproc_t *)data;

  gavl_time_t delay_time = GAVL_TIME_SCALE / 20;

  gavl_audio_format_t audio_format;
  gavl_video_format_t video_format;
  
  
  /* We add and remove our message queue at the beginning and
     end of the thread */
  
  bg_player_add_message_queue(l->player,
                              l->queue);
  
  while(1)
    {
    /* Check if we must stop the thread */

    pthread_mutex_lock(&(l->state_mutex));
    if(l->do_stop)
      {
      pthread_mutex_unlock(&(l->state_mutex));
      break;
      }
    pthread_mutex_unlock(&(l->state_mutex));
    
    /* Check whether to process a message */

    pthread_mutex_lock(&(l->config_mutex));
    while((msg = bg_msg_queue_try_lock_read(l->queue)))
      {
      id = bg_msg_get_id(msg);
      //      fprintf(stderr, "Got message: %d\n", id);
      
      switch(id)
        {
        case BG_PLAYER_MSG_TIME_CHANGED:
          if(l->have_name_time)
            {
            time = bg_msg_get_arg_time(msg, 0);
            set_time(l, time);
            }
          break;
        case BG_PLAYER_MSG_STATE_CHANGED:
          arg_i_1 = bg_msg_get_arg_int(msg, 0);
          switch(arg_i_1)
            {
            case BG_PLAYER_STATE_STOPPED:
            case BG_PLAYER_STATE_CHANGING:
              if(l->have_formats)
                {
                set_audio_format(l, NULL);
                set_video_format(l, NULL);
                }
              if(l->have_descriptions)
                {
                set_audio_description(l, NULL);
                set_video_description(l, NULL);
                }
              if(l->have_name_time)
                {
                set_time(l, GAVL_TIME_UNDEFINED);
                set_name(l, PACKAGE"-"VERSION);
                }
              break;
            }
          break;
        case BG_PLAYER_MSG_TRACK_NAME:
          if(l->have_name_time)
            {
            arg_str_1 = bg_msg_get_arg_string(msg, 0);
            set_name(l, arg_str_1);
            fprintf(stderr, "BG_PLAYER_MSG_TRACK_NAME %s\n", arg_str_1);
            free(arg_str_1);
            }
          break;
        case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
          if(l->have_descriptions)
            {
            arg_str_1 = bg_msg_get_arg_string(msg, 0);
            set_audio_description(l, arg_str_1);
            free(arg_str_1);
            }
          break;
        case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
          if(l->have_descriptions)
            {
            arg_str_1 = bg_msg_get_arg_string(msg, 0);
            set_video_description(l, arg_str_1);
            free(arg_str_1);
            }
          break;
        case BG_PLAYER_MSG_AUDIO_STREAM:
          if(l->have_formats)
            {
            bg_msg_get_arg_audio_format(msg, 1, &(audio_format));
            set_audio_format(l, &(audio_format));
            }
          break;
        case BG_PLAYER_MSG_VIDEO_STREAM:
          if(l->have_formats)
            {
            bg_msg_get_arg_video_format(msg, 1, &(video_format));
            set_video_format(l, &(video_format));
            }
          break;
        }
      bg_msg_queue_unlock_read(l->queue);
      }
    pthread_mutex_unlock(&(l->config_mutex));
    
    /* Sleep */
    gavl_time_delay(&delay_time);
    }

  bg_player_delete_message_queue(l->player,
                                 l->queue);

  return NULL;
  }

static void start_thread(bg_lcdproc_t * l)
  {
  fprintf(stderr, "Start thread...");
  pthread_mutex_lock(&(l->state_mutex));

  pthread_create(&(l->thread), (pthread_attr_t*)0,
                 thread_func, l);

  l->is_running = 1;
  pthread_mutex_unlock(&(l->state_mutex));
  fprintf(stderr, "done");
  }

static void stop_thread(bg_lcdproc_t * l)
  {
  fprintf(stderr, "Stop thread...");
  pthread_mutex_lock(&(l->state_mutex));
  l->do_stop = 1;

  if(!l->is_running)
    {
    pthread_mutex_unlock(&(l->state_mutex));
    return;
    }

  pthread_mutex_unlock(&(l->state_mutex));

  pthread_join(l->thread, (void**)0);

  pthread_mutex_lock(&(l->state_mutex));
  l->do_stop = 0;
  l->is_running = 0;
  pthread_mutex_unlock(&(l->state_mutex));
  fprintf(stderr, "done");
  }

/*
 *  Config stuff. The function set_parameter automatically
 *  starts and stops the thread
 */

bg_parameter_info_t * bg_lcdproc_get_parameters(bg_lcdproc_t * l)
  {
  return parameters;
  }

#define IPARAM(v) if(!strcmp(name, #v)) { l->v = val->val_i; }

void bg_lcdproc_set_parameter(void * data, char * name,
                              bg_parameter_value_t * val)
  {
  bg_lcdproc_t * l = (bg_lcdproc_t *)data;

  if(!name)
    {
    if(l->fd >= 0)
      stop_thread(l);

    /* Disable everything we don't want to show */

    //    if(!l->
    
    /* Check wether to disconnect */
        
    if((l->fd >= 0) &&
       ((l->port != l->port_cfg) ||
        strcmp(l->hostname, l->hostname_cfg) ||
        !l->enable_lcdproc))
      {
      close(l->fd);
      l->fd = -1;
      
      l->have_formats      = 0;
      l->have_name_time    = 0;
      l->have_descriptions = 0;
      }

    if(!l->enable_lcdproc)
      {
      return;
      }
    
    /* Switch off widgets we don't want */

    if(!l->display_name_time && l->have_name_time)
      destroy_name_time(l);
    
    if(!l->display_formats && l->have_formats)
      destroy_formats(l);
    
    if(!l->display_descriptions && l->have_descriptions)
      destroy_descriptions(l);
    
    if(l->enable_lcdproc)
      {
      /* Check whether to reconnect */
      if((l->fd < 0) && !do_connect(l))
        return;
      
      if(l->display_name_time && !l->have_name_time)
        create_name_time(l);

      if(l->display_formats && !l->have_formats)
        create_formats(l);

      if(l->display_descriptions && !l->have_descriptions)
        create_descriptions(l);

      start_thread(l);
      }
    return;
    }
  
  if(!strcmp(name, "enable_lcdproc"))
    {
    l->enable_lcdproc = val->val_i;
    }
  else if(!strcmp(name, "hostname"))
    {
    l->hostname_cfg = bg_strdup(l->hostname_cfg, val->val_str);
    }
  else if(!strcmp(name, "port"))
    {
    l->port_cfg = val->val_i;
    }

  pthread_mutex_lock(&(l->config_mutex));
  IPARAM(display_name_time);
  IPARAM(display_formats);
  IPARAM(display_descriptions);
  pthread_mutex_unlock(&(l->config_mutex));
  }
