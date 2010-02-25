/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>


#include <limits.h>

#include <config.h>
#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "utils"

char * bg_search_file_read(const char * directory, const char * file)
  {
  char * testpath;
  char * home_dir;
  FILE * testfile;

  if(!file)
    return (char*)0;
  
  testpath = malloc(FILENAME_MAX * sizeof(char));
  
  /* First step: Try home directory */
  
  home_dir = getenv("HOME");
  if(home_dir)
    {
    sprintf(testpath, "%s/.%s/%s/%s", home_dir,
            PACKAGE, directory, file);
    testfile = fopen(testpath, "r");
    if(testfile)
      {
      fclose(testfile);
      return testpath;
      }
    }
  /* Second step: Try Data directory */
  sprintf(testpath, "%s/%s/%s", DATA_DIR, directory, file);
  testfile = fopen(testpath, "r");
  if(testfile)
    {
    fclose(testfile);
    return testpath;
    }
  free(testpath);
  return (char*)0;
  }

char * bg_search_file_write(const char * directory, const char * file)
  {
  char * home_dir;
  char * testpath;
  char * testdir;
  
  FILE * testfile;

  if(!file)
    return (char*)0;
  
  home_dir = getenv("HOME");

  /* Try to open the file */

  testdir  = bg_sprintf("%s/.%s/%s", home_dir, PACKAGE, directory);

  if(!bg_ensure_directory(testdir))
    {
    free(testdir);
    return NULL;
    }
  
  testpath = bg_sprintf("%s/%s", testdir, file);
  
  testfile = fopen(testpath, "a");
  if(testfile)
    {
    fclose(testfile);
    free(testdir);
    return testpath;
    }
  else
    {
    free(testpath);
    free(testdir);
    return (char*)0;
    }
  }

int bg_ensure_directory(const char * dir)
  {
  char ** directories;
  char * subpath = (char*)0;
  int i, ret;
  
  /* Return early */
  if(!access(dir, R_OK|W_OK|X_OK))
    return 1;
  
  /* We omit the first slash */
  directories = bg_strbreak(dir+1, '/');

  i = 0;
  ret = 1;
  while(directories[i])
    {
    subpath = bg_strcat(subpath, "/");
    subpath = bg_strcat(subpath, directories[i]);

    if(access(subpath, R_OK) && (errno == ENOENT))
      {
      if(mkdir(subpath, S_IRUSR|S_IWUSR|S_IXUSR) == -1)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating directory %s failed: %s",
               subpath, strerror(errno));
        ret = 0;
        break;
        }
      else
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created directory %s", subpath);
      }
    i++;
    }
  if(subpath)
    free(subpath);
  bg_strbreak_free(directories);
  return ret;
  }

int bg_search_file_exec(const char * file, char ** _path)
  {
  int i;
  char * path;
  char ** searchpaths;
  char * test_filename;
  
  struct stat st;

  /* Try the dependencies path first */
  test_filename = bg_sprintf("/opt/gmerlin/bin/%s", file);
  if(!stat(test_filename, &st) && (st.st_mode & S_IXOTH))
    {
    if(_path)
      *_path = test_filename;
    else
      free(test_filename);
    return 1;
    }
  free(test_filename);
  
  path = getenv("PATH");
  if(!path)
    return 0;

  searchpaths = bg_strbreak(path, ':');
  i = 0;
  while(searchpaths[i])
    {
    test_filename = bg_sprintf("%s/%s", searchpaths[i], file);
    if(!stat(test_filename, &st) && (st.st_mode & S_IXOTH))
      {
      if(_path)
        *_path = test_filename;
      else
        free(test_filename);
      bg_strbreak_free(searchpaths);
      return 1;
      }
    free(test_filename);
    i++;
    }
  bg_strbreak_free(searchpaths);
  return 0;
  }

static const struct
  {
  char * command;
  char * template;
  }
webbrowsers[] =
  {
    { "firefox", "firefox %s" },
    { "mozilla", "mozilla %s" },
  };

char * bg_find_url_launcher()
  {
  int i;
  char * ret = (char*)0;
  int ret_alloc = 0;
  bg_subprocess_t * proc;
  /* Try to get the default url handler from gnome */
  
  if(bg_search_file_exec("gconftool-2", (char**)0))
    {
    proc =
      bg_subprocess_create("gconftool-2 -g /desktop/gnome/url-handlers/http/command",
                           0, 1, 0);
    
    if(bg_subprocess_read_line(proc->stdout_fd, &ret, &ret_alloc, 0))
      {
      bg_subprocess_close(proc);
      return ret;
      }
    else if(ret)
      free(ret);
    bg_subprocess_close(proc);
    }
  for(i = 0; i < sizeof(webbrowsers)/sizeof(webbrowsers[0]); i++)
    {
    if(bg_search_file_exec(webbrowsers[i].command, (char**)0))
      {
      return bg_strdup((char*)0, webbrowsers[i].template);
      }
    }
  return (char*)0;
  }

void bg_display_html_help(const char * path)
  {
  char * url_launcher;
  char * complete_path;
  char * command;
  url_launcher = bg_find_url_launcher();
  if(!url_launcher)
    return;
  
  complete_path = bg_sprintf("file://%s/%s", DOC_DIR, path);
  command = bg_sprintf(url_launcher, complete_path);
  command = bg_strcat(command, " &");
  system(command);
  free(command);
  free(url_launcher);
  free(complete_path);
  }
