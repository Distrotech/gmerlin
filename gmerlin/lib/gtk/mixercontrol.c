/*****************************************************************
 
  mixercontrol.c
 
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>

#include <cfg_dialog.h>
#include <gui_gtk/audio.h>
#include <utils.h>

/* Utility functions */

static GtkWidget * create_pixmap_button(const char * filename)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  return button;
  }

static GtkWidget * create_pixmap_toggle_button(const char * filename)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);

  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();
    
  gtk_widget_show(image);
  button = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  return button;
  }

static void set_image(GtkWidget * image, const char * filename)
  {
  char * path;
  path = bg_search_file_read("icons", filename);
  gtk_image_set_from_file(GTK_IMAGE(image), path);
  if(path)
    free(path);
  }

/* Configuration parameters */

static char * channel_icon_files[] =
  {
    NULL,
    "bass_32.png",
    "treble_32.png",
    "cd_32.png",
    "record_32.png",
    "speaker_32.png",
    "mixer_32.png",
    "pcm_32.png",
    "line_32.png",
    "mic_32.png",
    "video_32.png",
    NULL
  };


static char * channel_icon_names[] =
  {
    "None",
    "Bass",
    "Treble",
    "CD",
    "Record",
    "Speaker",
    "Mixer",
    "PCM",
    "Line in",
    "Microphone",
    "Video",
    NULL
  };

static bg_parameter_info_t parameter_record =
  {
    name:        "record",
    long_name:   "Record",
    type:        BG_PARAMETER_CHECKBUTTON,
    flags:       BG_PARAMETER_SYNC,
    val_default: { val_i: 0 },
  };

static bg_parameter_info_t parameter_mute =
  {
    name:        "mute",
    long_name:   "Mute",
    type:        BG_PARAMETER_CHECKBUTTON,
    flags:       BG_PARAMETER_SYNC,
    val_default: { val_i: 0 },
  };

static bg_parameter_info_t parameter_volume_mono =
  {
    name:      "value",
    long_name: "Value",
    type:      BG_PARAMETER_SLIDER_FLOAT,
    flags:     BG_PARAMETER_SYNC,
    val_default: { val_f: 0.5 },
    val_min:     { val_f: 0.0 },
    val_max:     { val_f: 1.0 },
    num_digits: 3,
  };

static bg_parameter_info_t parameter_volume_stereo[3] =
  {
    {
      name:      "value_l",
      long_name: "Value left",
      type:      BG_PARAMETER_SLIDER_FLOAT,
      flags:     BG_PARAMETER_SYNC,
      val_default: { val_f: 0.5 },
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 1.0 },
      num_digits: 3,
    },
    {
      name:      "value_r",
      long_name: "Value right",
      type:      BG_PARAMETER_SLIDER_FLOAT,
      flags:     BG_PARAMETER_SYNC,
      val_default: { val_f: 0.5 },
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 1.0 },
      num_digits: 3,
    },
    {
      name:        "locked",
      long_name:   "Lock Channels",
      type:        BG_PARAMETER_CHECKBUTTON,
      flags:       BG_PARAMETER_SYNC,
      val_default: { val_i: 1 },
    },
  };

static bg_parameter_info_t parameter_label =
  {
      name:      "label",
      long_name: "Label",
      type:      BG_PARAMETER_STRING,
  };

static bg_parameter_info_t parameter_icon =
  {
    name:        "icon",
    long_name:   "Icon",
    type:        BG_PARAMETER_STRINGLIST,
    val_default: { val_str: "Speaker" },
    options:     channel_icon_names,
  };


struct bg_gtk_mixer_control_s
  {
  GtkWidget * slider_l;
  GtkWidget * slider_r;

  GtkWidget * record_button;
  GtkWidget * mute_button;
  GtkWidget * lock_button;
  GtkWidget * config_button;
  
  GtkObject * adj_l;
  GtkObject * adj_r;

  GtkWidget * label;
  GtkWidget * table;
  GtkWidget * handlebox;
  GtkWidget * icon;
  gulong handler_id_l;
  gulong handler_id_r;
  gulong handler_id_record;
  
  bg_gtk_mixer_callbacks_t * callbacks;
  bg_cfg_section_t * config_section;

  bg_parameter_info_t * parameters;
    
  void * callback_data;
  int locked;
  int muted;
  float volume[2];
  };

static void set_parameter(void * data, char * name,
                          bg_parameter_value_t * v)
  {
  int i;
  bg_gtk_mixer_control_t * ctrl = (bg_gtk_mixer_control_t*)data;

  /*  fprintf(stderr, "Set parameter %s\n", name); */
  if(!name)
    return;
  if(!strcmp(name, "value_l"))
    {
    ctrl->volume[0] = v->val_f;
    bg_gtk_mixer_control_set_value(ctrl, ctrl->volume, 1);
    }
  else if(!strcmp(name, "value_r"))
    {
    ctrl->volume[1] = v->val_f;
    bg_gtk_mixer_control_set_value(ctrl, ctrl->volume, 1);
    }
  if(!strcmp(name, "value"))
    {
    ctrl->volume[0] = v->val_f;
    bg_gtk_mixer_control_set_value(ctrl, ctrl->volume, 1);
    }
  else if(!strcmp(name, "label"))
    {
    gtk_label_set_text(GTK_LABEL(ctrl->label), v->val_str);
    }
  else if(!strcmp(name, "icon"))
    {
    if(v->val_str)
      {
      i = 0;
      while(channel_icon_names[i])
        {
        if(!strcmp(channel_icon_names[i], v->val_str))
          break;
        i++;
        }
      set_image(ctrl->icon, channel_icon_files[i]);
      }
    else
      set_image(ctrl->icon, NULL);
    }
  else if(!strcmp(name, "locked"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl->lock_button), v->val_i);
    }
  else if(!strcmp(name, "mute"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl->mute_button), v->val_i);
    }
  else if(!strcmp(name, "record"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl->record_button), v->val_i);
    }
  
  }

static void change_callback(GtkObject * adj, gpointer data)
  {
  bg_gtk_mixer_control_t * v = (bg_gtk_mixer_control_t*)data;

  /*  fprintf(stderr, "Change callback\n"); */
    
  if(adj == v->adj_l)
    {
    v->volume[0] = 1.0-gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_l));
    if(v->locked)
      {
      v->volume[1] = v->volume[0];
      g_signal_handler_block(v->adj_r, v->handler_id_r);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(v->adj_r),
                               gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_l)));
      g_signal_handler_unblock(v->adj_r, v->handler_id_r);
      }
    }
  else if(adj == v->adj_r)
    {
    v->volume[1] = 1.0-gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_r));
    if(v->locked)
      {
      v->volume[0] = v->volume[1];
      g_signal_handler_block(v->adj_l, v->handler_id_l);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(v->adj_l),
                               gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_r)));
      g_signal_handler_unblock(v->adj_l, v->handler_id_l);
      }
    }
  
  if(v->callbacks->change && !v->muted)
    v->callbacks->change(v, v->volume, v->callback_data);
  }

static void put_config(bg_gtk_mixer_control_t * v)
  {
  bg_parameter_value_t val;
  int parameter_index = 0;
  while(v->parameters[parameter_index].name)
    {
    if(!strcmp(v->parameters[parameter_index].name, "value"))
      {
      val.val_f = v->volume[0];
      bg_cfg_section_set_parameter(v->config_section,
                                   &(v->parameters[parameter_index]),
                                   &val);
      }
    else if(!strcmp(v->parameters[parameter_index].name, "value_l"))
      {
      val.val_f = v->volume[0];
      bg_cfg_section_set_parameter(v->config_section,
                                   &(v->parameters[parameter_index]),
                                   &val);
      }
    else if(!strcmp(v->parameters[parameter_index].name, "value_r"))
      {
      val.val_f = v->volume[1];
      bg_cfg_section_set_parameter(v->config_section,
                                   &(v->parameters[parameter_index]),
                                   &val);
      }
    else if(!strcmp(v->parameters[parameter_index].name, "mute"))
      {
      val.val_i = v->muted;
      bg_cfg_section_set_parameter(v->config_section,
                                   &(v->parameters[parameter_index]),
                                   &val);
        
      }
    else if(!strcmp(v->parameters[parameter_index].name, "locked"))
      {
      val.val_i = v->locked;
      bg_cfg_section_set_parameter(v->config_section,
                                   &(v->parameters[parameter_index]),
                                   &val);
        
      }
    else if(!strcmp(v->parameters[parameter_index].name, "record"))
      {
      val.val_i =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(v->record_button));
      ;
      bg_cfg_section_set_parameter(v->config_section,
                                   &(v->parameters[parameter_index]),
                                   &val);
        
      }
    parameter_index++;
    }
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  float val[2];
  bg_dialog_t * dialog;
  
  bg_gtk_mixer_control_t * v = (bg_gtk_mixer_control_t*)data;

  if(w == v->config_button)
    {
    put_config(v);
    dialog = bg_dialog_create(v->config_section,
                              set_parameter,
                              v, v->parameters);
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }
  else if(w == v->lock_button)
    {
    v->locked = !v->locked;
    if(v->locked)
      {
      g_signal_handler_block(v->adj_l, v->handler_id_l);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(v->adj_l),
                               gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_r)));
      g_signal_handler_unblock(v->adj_l, v->handler_id_l);
      }
    }
  else if(w == v->mute_button)
    {
    v->muted = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(v->mute_button));
    if(v->callbacks->set_mute)
      v->callbacks->set_mute(v, v->muted, v->callback_data);
    else if(v->callbacks->change)
      {
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(v->mute_button)))
        {
        val[0] = 0.0;
        val[1] = 0.0;
        v->callbacks->change(v, val, v->callback_data);
        }
      else
        {
        v->callbacks->change(v, v->volume, v->callback_data);
        }
      
      }
    }
  else if(w == v->record_button)
    {
    if(v->callbacks->set_record)
      v->callbacks->set_record(v,
                               gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(v->record_button)),
                               v->callback_data);
    
    }
      
  }

static void setup_scale(GtkWidget * scale)
  {
  int width, height;

  gtk_widget_get_size_request(scale, &width, &height);
  gtk_widget_set_size_request(scale, width, 200);
  
  gtk_scale_set_digits(GTK_SCALE(scale), 3);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  }

bg_gtk_mixer_control_t *
bg_gtk_mixer_control_create(const char *label, int stereo,
                            int record,
                            bg_gtk_mixer_callbacks_t * callbacks,
                            void * callback_data)
  {
  int num_parameters;
  int parameter_index;
  
  bg_gtk_mixer_control_t * ret = calloc(1, sizeof(*ret));
  
  ret->callbacks = callbacks;
  ret->callback_data = callback_data;

  ret->label = gtk_label_new(label);
  
  gtk_widget_show(ret->label);

  ret->icon = gtk_image_new();
  gtk_widget_set_size_request(ret->icon, 32, 32);
  gtk_widget_show(ret->icon);

  if(record)
    {
    ret->record_button = create_pixmap_toggle_button("record_16.png");
    ret->handler_id_record =
      g_signal_connect(G_OBJECT(ret->record_button),
                       "toggled", G_CALLBACK(button_callback),
                       (gpointer)ret);
    gtk_widget_show(ret->record_button);
    }
  ret->mute_button   = create_pixmap_toggle_button("mute_16.png");
  g_signal_connect(G_OBJECT(ret->mute_button),
                     "toggled", G_CALLBACK(button_callback),
                     (gpointer)ret);
  
  gtk_widget_show(ret->mute_button);
  
  if(stereo)
    {
    ret->lock_button   = create_pixmap_toggle_button("lock_16.png");
    g_signal_connect(G_OBJECT(ret->lock_button),
                       "toggled", G_CALLBACK(button_callback),
                       (gpointer)ret);
    gtk_widget_show(ret->lock_button);
    }
  ret->config_button = create_pixmap_button("config_16.png");
  g_signal_connect(G_OBJECT(ret->config_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)ret);
  gtk_widget_show(ret->config_button);
  
  ret->adj_l = gtk_adjustment_new(0.0, 0.0, 1.0,
                                  1.0, 0.0, 0.0);
  ret->handler_id_l = g_signal_connect(G_OBJECT(ret->adj_l), "value_changed",
                                       G_CALLBACK(change_callback),
                                       (gpointer)ret);
  
  ret->slider_l = gtk_vscale_new(GTK_ADJUSTMENT(ret->adj_l));
  
  setup_scale(ret->slider_l);
  
  
  gtk_widget_show(ret->slider_l);

  if(stereo)
    {
    ret->adj_r = gtk_adjustment_new(0.0, 0.0, 1.0,
                                    1.0, 0.0, 0.0);
    ret->handler_id_r = g_signal_connect(G_OBJECT(ret->adj_r), "value_changed",
                                         G_CALLBACK(change_callback),
                                         (gpointer)ret);
    
    ret->slider_r = gtk_vscale_new(GTK_ADJUSTMENT(ret->adj_r));
    setup_scale(ret->slider_r);
        
    gtk_widget_show(ret->slider_r);
    }


  ret->table = gtk_table_new(5, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(ret->table), 3);

  gtk_table_attach(GTK_TABLE(ret->table), ret->label,
                   0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(ret->table), ret->icon,
                   0, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  
  if(stereo)
    {
    gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->slider_l,
                              0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->slider_r,
                              1, 2, 2, 3);
    gtk_table_attach(GTK_TABLE(ret->table), ret->lock_button,
                     1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(ret->table), ret->mute_button,
                     0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
    }
  else
    {
    gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->slider_l,
                              0, 2, 2, 3);
    gtk_table_attach(GTK_TABLE(ret->table), ret->mute_button,
                     0, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
    }
  
  if(record)
    {
    gtk_table_attach(GTK_TABLE(ret->table), ret->record_button,
                     1, 2, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(ret->table), ret->config_button,
                     0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
    }
  else
    gtk_table_attach(GTK_TABLE(ret->table), ret->config_button,
                     0, 2, 4, 5, GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show(ret->table);

  ret->handlebox = gtk_handle_box_new();
  gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(ret->handlebox),
                                     GTK_POS_TOP);
  
  gtk_container_add(GTK_CONTAINER(ret->handlebox), ret->table);
  gtk_widget_show(ret->handlebox);
  
  /* Create parameter infos */

  num_parameters = 4; /* Label, Icon, Mute, Value */
  if(stereo)
    num_parameters += 2; /* Lock, Value */
  if(record)
    num_parameters++; /* Record */

  ret->parameters = calloc(num_parameters+1, sizeof(*(ret->parameters)));

  parameter_index = 0;

  if(stereo)
    {
    bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                           &(parameter_volume_stereo[0]));
    parameter_index++;

    bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                           &(parameter_volume_stereo[1]));
    parameter_index++;

    bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                           &(parameter_volume_stereo[2]));
    parameter_index++;
    }
  else
    {
    bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                           &(parameter_volume_mono));
    parameter_index++;
    }
  bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                         &(parameter_mute));
  parameter_index++;

  bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                         &(parameter_label));

  ret->parameters[parameter_index].val_default.val_str =
    malloc(strlen(label)+1);
  strcpy(ret->parameters[parameter_index].val_default.val_str, label);

  parameter_index++;

  bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                         &(parameter_icon));
  
  
  if(record)
    {
    parameter_index++;
    bg_parameter_info_copy(&(ret->parameters[parameter_index]),
                           &(parameter_record));
    }
  
  
  return ret;
  };

void bg_gtk_mixer_control_configure(bg_gtk_mixer_control_t * ctrl,
                                    bg_cfg_section_t * section)
  {
  ctrl->config_section = section;
  bg_cfg_section_apply(section, ctrl->parameters, set_parameter, (void*)ctrl);
  }
                                    
GtkWidget *
bg_gtk_mixer_control_get_widget(bg_gtk_mixer_control_t * v)
  {
  return v->handlebox;
  }

void
bg_gtk_mixer_control_destroy(bg_gtk_mixer_control_t * v)
  {
  /*  fprintf(stderr, "Destroy mixercontrol\n"); */
  
  if(v->config_section)
    {
    put_config(v);
    }
  bg_parameter_info_destroy_array(v->parameters);
  free(v);
  }

void
bg_gtk_mixer_control_set_value(bg_gtk_mixer_control_t * v,
                                float * value,
                                int send_callback)
  {
/*   if(!send_callback) */
/*     g_signal_handler_block(v->adj_r, v->handler_id_l); */
  gtk_adjustment_set_value(GTK_ADJUSTMENT(v->adj_l), 1.0-value[0]);

/*   if(!send_callback) */
/*     g_signal_handler_unblock(v->adj_r, v->handler_id_l); */

  if(v->adj_r)
    {
/*     if(!send_callback) */
/*       g_signal_handler_block(v->adj_r, v->handler_id_r); */

    gtk_adjustment_set_value(GTK_ADJUSTMENT(v->adj_r), 1.0-value[1]);

/*     if(!send_callback) */
/*       g_signal_handler_unblock(v->adj_r, v->handler_id_r); */
    }
  }

void
bg_gtk_mixer_control_set_record(bg_gtk_mixer_control_t * m, int record)
  {
  if(!m->record_button)
    return;
  
  g_signal_handler_block(m->record_button, m->handler_id_record);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m->record_button),
                               record);
  g_signal_handler_unblock(m->record_button, m->handler_id_record);
  }


void
bg_gtk_mixer_control_get_value(bg_gtk_mixer_control_t * v, float * ret)
  {
  ret[0] = gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_l));
  if(v->adj_r)
    ret[1] = gtk_adjustment_get_value(GTK_ADJUSTMENT(v->adj_r));
  }

