/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#include <config.h>

#include <translation.h>
#include <parameter.h>
#include <player.h>
#include <lcdproc.h>
#include <bgsocket.h>

#include <string.h>
#include <sys/types.h> /* pid_t */
#include <unistd.h>
#include <utils.h>

#include <log.h>
#define LOG_DOMAIN "lcdproc"

static const char * const formats_name      = "formats";
static const char * const name_time_name    = "name_time";
static const char * const descriptions_name = "descriptions";

static const char * const audio_format_name        = "audio_format";
static const char * const video_format_name        = "video_format";

static const char * const audio_description_name   = "audio_description";
static const char * const video_description_name   = "video_description";

static const char * const name_name                = "name";
static const char * const time_name                = "time";

struct bg_lcdproc_s
  {
  int player_state;
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

  /* Screen size (got from server welcome line) */
  
  int width, height;
  
  bg_player_t * player;
  };

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "enable_lcdproc",
      .long_name =   TRS("Enable LCDproc"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
    {
      .name = "hostname",
      .long_name = TRS("Hostname"),
      .type = BG_PARAMETER_STRING,
      .val_default = { .val_str = "localhost" },
    },
    {
      .name =        "port",
      .long_name =   TRS("Port"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 1024 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 13666 },
    },
    {
      .name = "display_name_time",
      .long_name = TRS("Display track name and time"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name = "display_formats",
      .long_name = TRS("Display audio/video formats"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name = "display_descriptions",
      .long_name = TRS("Display audio/video descriptions"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
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

static int send_command(bg_lcdproc_t * l, char * command)
  {
  char nl = '\n';
  //  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Sending command %s", command);
  if(!bg_socket_write_data(l->fd, (uint8_t*)command, strlen(command)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Sending command failed");
    return 0;
    }
  if(!bg_socket_write_data(l->fd, (uint8_t*)&nl, 1))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Sending command failed");
    return 0;
    }


  while(1)
    {
    if(!bg_socket_read_line(l->fd, &(l->answer), &(l->answer_alloc), 0))
      {
      //      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading answer failed");
      //      return 0;
      break;
      }
    
    //    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Got answer %s", l->answer);

    if(!strncmp(l->answer, "success", 7))
      break;
    else if(!strncmp(l->answer, "listen", 6))
      break;
    else if(!strncmp(l->answer, "huh", 3))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Command \"%s\" not unserstood by server",
             command);
      return 0;
      }
    }
  
  return 1;
  }

/* Here comes the real working stuff */

static int do_connect(bg_lcdproc_t* l)
  {
  int i;
  char ** answer_args;
  char * cmd = (char*)0;
  
  bg_host_address_t * addr = bg_host_address_create();

  if(!bg_host_address_set(addr, l->hostname_cfg, l->port_cfg, SOCK_STREAM))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not resolve adress for: %s",
           l->hostname_cfg);
    goto fail;
    }
  l->fd = bg_socket_connect_inet(addr, 500);
  if(l->fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not connect to server at %s:%d",
           l->hostname_cfg, l->port_cfg);
    goto fail;
    }

  //  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Sending hello");
  
  /* Send hello and get answer */
  if(!bg_socket_write_data(l->fd, (uint8_t*)"hello\n", 6))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not send hello message");
    goto fail;
    }
  
  if(!bg_socket_read_line(l->fd, &(l->answer), &(l->answer_alloc), 1000))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not get server welcome line");
    goto fail;
    }

  //  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Got welcome line: %s", l->answer);

  
  if(strncmp(l->answer, "connect LCDproc", 15))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid answer: %s", l->answer);
    goto fail;
    }

  answer_args = bg_strbreak(l->answer, ' ');
  i = 0;
  while(answer_args[i])
    {
    if(!strcmp(answer_args[i], "wid"))
      {
      i++;
      l->width = atoi(answer_args[i]);
      }
    else if(!strcmp(answer_args[i], "hgt"))
      {
      i++;
      l->height = atoi(answer_args[i]);
      }
    i++;
    }
  bg_strbreak_free(answer_args);
  
  
  /* Set client attributes */

  cmd = bg_sprintf("client_set name {gmerlin%d}", getpid());
  
  if(!send_command(l, cmd))
    goto fail;
  free(cmd);
  
  bg_log(BG_LOG_INFO, LOG_DOMAIN,
         "Connection to server established, display size: %dx%d",
         l->width, l->height);
  
  return 1;
  fail:
  bg_host_address_destroy(addr);

  if(cmd) free(cmd);
  
  if(l->fd >= 0)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Connection to server closed due to error");
    
    close(l->fd);
    l->fd = -1;
    }
  return 0;
  }

static int set_name(bg_lcdproc_t * l, const char * name)
  {
  char * command;

  if(strlen(name) > l->width)
    command = bg_sprintf("widget_set %s %s 1 2 %d 3 m 1 {%s *** }",
                       name_time_name, name_name, l->width, name);
  else
    command = bg_sprintf("widget_set %s %s 1 2 %d 3 m 1 {%s}",
                       name_time_name, name_name, l->width, name);
  
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
  char * command, *format;
  char buffer[16]; /* MUST be larger than GAVL_TIME_STRING_LEN */
  
  if(time == GAVL_TIME_UNDEFINED)
    {
    gethostname(buffer, 16);
    
    command = bg_sprintf("widget_set %s %s 1 1 {%s}",
                         name_time_name, time_name, buffer);
    if(!send_command(l, command))
      goto fail;
    }
  else
    {
    gavl_time_prettyprint(time, buffer);

    format = bg_sprintf("widget_set %%s %%s 1 1 {T: %%%ds}", l->width - 3);
        
    command = bg_sprintf(format,
                         name_time_name, time_name, buffer);
    if(!send_command(l, command))
      goto fail;
    free(format);
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
  command = bg_sprintf("screen_add %s", name_time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  command = bg_sprintf("screen_set %s -heartbeat off", name_time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  
  /* Time display */
  
  command = bg_sprintf("widget_add %s %s string",
                       name_time_name, time_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_time(l, GAVL_TIME_UNDEFINED);
  
  /* Name display */
  command = bg_sprintf("widget_add %s %s scroller",
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

  command = bg_sprintf("screen_del %s", name_time_name);
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
  char * format_string;
  
  if(!f)
    {
    command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {Audio: none}",
                         formats_name, audio_format_name);
    }
  else
    {
    if(f->num_channels == 1)
      format_string = bg_sprintf("%d Hz Mono", f->samplerate);
    else if(f->num_channels == 2)
      format_string = bg_sprintf("%d Hz Stereo", f->samplerate);
    else
      format_string = bg_sprintf("%d Hz %d Ch", f->samplerate, f->num_channels);

    if(strlen(format_string) > l->width)
      command = 
        bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {%s *** }",
                   formats_name, audio_format_name, format_string);
    else
      command = 
        bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {%s}",
                   formats_name, audio_format_name, format_string);

    free(format_string);
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
  char * command, *format_string;
  if(!f)
    {
    command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {Video: none}",
                         formats_name, video_format_name);
    }
  else
    {
    
    if(f->framerate_mode == GAVL_FRAMERATE_CONSTANT)
      {
      format_string = bg_sprintf("%dx%d %.2f fps", f->image_width,
                                 f->image_height, (float)(f->timescale)/
                                 (float)(f->frame_duration));
      
      }
    else
      {
      format_string = bg_sprintf("%dx%d", f->image_width,
                                 f->image_height);
      }

    if(strlen(format_string) > l->width)
      command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {%s *** }",
                           formats_name, video_format_name, format_string);
    else
      command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {%s}",
                           formats_name, video_format_name, format_string);
    free(format_string);
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
  command = bg_sprintf("screen_add %s", formats_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  command = bg_sprintf("screen_set %s -heartbeat off", formats_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  
  /* Audio format */

  command = bg_sprintf("widget_add %s %s scroller",
                       formats_name, audio_format_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_audio_format(l, NULL);
  
  /* Video format */
  
  command = bg_sprintf("widget_add %s %s scroller",
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
  
  command = bg_sprintf("screen_del %s", formats_name);
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
    command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {A: Off}",
                         descriptions_name, audio_description_name);
    }
  else
    {
    if(strlen(desc) > l->width)
      command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {A: %s *** }",
                           descriptions_name, audio_description_name, desc);
      
    else
      command = bg_sprintf("widget_set %s %s 1 1 16 2 m 1 {A: %s}",
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
    command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {V: Off}",
                         descriptions_name, video_description_name);
    }
  else
    {
    if(strlen(desc) > l->width)
      command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {V: %s *** }",
                           descriptions_name, video_description_name, desc);
    else
      command = bg_sprintf("widget_set %s %s 1 2 16 3 m 1 {V: %s}",
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
  command = bg_sprintf("screen_add %s", descriptions_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  command = bg_sprintf("screen_set %s -heartbeat off", descriptions_name);
  if(!send_command(l, command))
    goto fail;
  free(command);
  
  /* Audio description */

  command = bg_sprintf("widget_add %s %s scroller",
                       descriptions_name, audio_description_name);
  if(!send_command(l, command))
    goto fail;
  free(command);

  set_audio_description(l, NULL);
  
  /* Video format */
  
  command = bg_sprintf("widget_add %s %s scroller",
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
  
  command = bg_sprintf("screen_del %s", descriptions_name);
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
  int result = 1;
  
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
      if(l->fd < 0)
        {
        gavl_time_delay(&delay_time);
        continue;
        }
      id = bg_msg_get_id(msg);
      
      switch(id)
        {
        case BG_PLAYER_MSG_TIME_CHANGED:
          if(l->have_name_time)
            {
            if(l->player_state != BG_PLAYER_STATE_STOPPED)
              time = bg_msg_get_arg_time(msg, 0);
            else
              time = GAVL_TIME_UNDEFINED;
            result = set_time(l, time);
            }
          break;
        case BG_PLAYER_MSG_STATE_CHANGED:
          arg_i_1 = bg_msg_get_arg_int(msg, 0);
          l->player_state = arg_i_1;
          switch(arg_i_1)
            {
            case BG_PLAYER_STATE_STOPPED:
            case BG_PLAYER_STATE_CHANGING:
              if(l->have_formats)
                {
                result = (set_audio_format(l, NULL) &&
                          set_video_format(l, NULL));
                }
              if(l->have_descriptions)
                {
                result = (set_audio_description(l, NULL) &&
                          set_video_description(l, NULL));
                }
              if(l->have_name_time)
                {
                result = (set_time(l, GAVL_TIME_UNDEFINED) &&
                          set_name(l, PACKAGE"-"VERSION));
                }
              break;
            }
          break;
        case BG_PLAYER_MSG_TRACK_NAME:
          if(l->have_name_time)
            {
            arg_str_1 = bg_msg_get_arg_string(msg, 0);
            result = set_name(l, arg_str_1);
            free(arg_str_1);
            }
          break;
        case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
          if(l->have_descriptions)
            {
            arg_str_1 = bg_msg_get_arg_string(msg, 0);
            result = set_audio_description(l, arg_str_1);
            free(arg_str_1);
            }
          break;
        case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
          if(l->have_descriptions)
            {
            arg_str_1 = bg_msg_get_arg_string(msg, 0);
            result = set_video_description(l, arg_str_1);
            free(arg_str_1);
            }
          break;
        case BG_PLAYER_MSG_AUDIO_STREAM:
          if(l->have_formats)
            {
            bg_msg_get_arg_audio_format(msg, 1, &(audio_format));
            result = set_audio_format(l, &(audio_format));
            }
          break;
        case BG_PLAYER_MSG_VIDEO_STREAM:
          if(l->have_formats)
            {
            bg_msg_get_arg_video_format(msg, 1, &(video_format));
            result = set_video_format(l, &(video_format));
            }
          break;
        }
      bg_msg_queue_unlock_read(l->queue);
      }
    pthread_mutex_unlock(&(l->config_mutex));
    
    /* Sleep */
    gavl_time_delay(&delay_time);

    if(!result && (l->fd >= 0))
      {
      close(l->fd);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Connection to server closed due to error");
      l->fd = -1;
      }
    
    }

  bg_player_delete_message_queue(l->player,
                                 l->queue);

  return NULL;
  }

static void start_thread(bg_lcdproc_t * l)
  {
  pthread_mutex_lock(&(l->state_mutex));

  pthread_create(&(l->thread), (pthread_attr_t*)0,
                 thread_func, l);

  l->is_running = 1;
  pthread_mutex_unlock(&(l->state_mutex));
  }

static void stop_thread(bg_lcdproc_t * l)
  {
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
  }

#define FREE(p) if(l->p) free(l->p);

void bg_lcdproc_destroy(bg_lcdproc_t* l)
  {
  stop_thread(l);
  FREE(answer);
  FREE(hostname);
  FREE(hostname_cfg);
  bg_msg_queue_destroy(l->queue);

  pthread_mutex_destroy(&(l->config_mutex));
  pthread_mutex_destroy(&(l->state_mutex));
  free(l);
  }


/*
 *  Config stuff. The function set_parameter automatically
 *  starts and stops the thread
 */

const bg_parameter_info_t * bg_lcdproc_get_parameters(bg_lcdproc_t * l)
  {
  return parameters;
  }

#define IPARAM(v) if(!strcmp(name, #v)) { l->v = val->val_i; }

void bg_lcdproc_set_parameter(void * data, const char * name,
                              const bg_parameter_value_t * val)
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
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Server connection closed");

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
