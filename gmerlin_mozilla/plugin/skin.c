#include <string.h>

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

static const char * default_skin_directory = GMERLIN_MOZILLA_DATA_DIR"/skins/Default";

static void widget_skin_load(bg_mozilla_widget_skin_t * s,
                             xmlDocPtr doc, xmlNodePtr node)
  {
  xmlNodePtr child;
  //  char * tmp_string;
  child = node->children;
  while(child)
    {
    if(!child->name)
      {
      child = child->next;
      continue;
      }
    else if(!BG_XML_STRCMP(child->name, "SEEKSLIDER"))
      {
      bg_gtk_slider_skin_load(&(s->seek_slider), doc, child);
      }
    else if(!BG_XML_STRCMP(child->name, "VOLUMESLIDER"))
      {
      bg_gtk_slider_skin_load(&(s->volume_slider), doc, child);
      }
    else if(!BG_XML_STRCMP(child->name, "PLAYBUTTON"))
      {
      bg_gtk_button_skin_load(&(s->play_button), doc, child);
      }
    else if(!BG_XML_STRCMP(child->name, "PAUSEBUTTON"))
      {
      bg_gtk_button_skin_load(&(s->pause_button), doc, child);
      }
    else if(!BG_XML_STRCMP(child->name, "STOPBUTTON"))
      {
      bg_gtk_button_skin_load(&(s->stop_button), doc, child);
      }
    else if(!BG_XML_STRCMP(child->name, "VOLUMEBUTTON"))
      {
      bg_gtk_button_skin_load(&(s->volume_button), doc, child);
      }
    child = child->next;
    }
  }


char * bg_mozilla_widget_skin_load(bg_mozilla_widget_skin_t * s,
                                   char * directory)
  {
  xmlNodePtr node;
  xmlNodePtr child;
  
  char * filename = (char*)0;
  xmlDocPtr doc = (xmlDocPtr)0;

  if(directory)
    {
    filename = bg_sprintf("%s/skin.xml", directory);
    doc = xmlParseFile(filename);
    }
  
  if(!doc)
    {
    free(filename);

    directory = bg_strdup(directory, default_skin_directory);
        
    filename = bg_sprintf("%s/skin.xml", directory);
    doc = xmlParseFile(filename);
    }

  if(!doc)
    {
    goto fail;
    }
    
  s->directory = bg_strdup(s->directory, directory);
  
  node = doc->children;
  
  if(BG_XML_STRCMP(node->name, "WEBPLUGINSKIN"))
    {
    goto fail;
    }
  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    child = node->children;

    if(!BG_XML_STRCMP(node->name, "PLUGINWIN"))
      widget_skin_load(s, doc, node);
        
    node = node->next;
    }

  fail:
  if(doc)
    xmlFreeDoc(doc);
  if(filename)
    free(filename);
  return directory;
  }


void bg_mozilla_widget_skin_destroy(bg_mozilla_widget_skin_t * s)
  {
  bg_gtk_slider_skin_free(&s->seek_slider);
  bg_gtk_slider_skin_free(&s->volume_slider);
  bg_gtk_button_skin_free(&s->stop_button);
  bg_gtk_button_skin_free(&s->pause_button);
  bg_gtk_button_skin_free(&s->play_button);
  }
