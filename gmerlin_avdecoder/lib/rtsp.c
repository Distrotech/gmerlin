/*****************************************************************
 
  in_rtsp.c
 
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
#include <sdp.h>
#include <http.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct bgav_rtsp_s
  {
  int fd;
  int cseq;
  
  char * user_agent;
  char * session;
  char * url;

  bgav_http_header_t * answers;
  
  bgav_sdp_t sdp;
  };

static void rtsp_send_request(bgav_rtsp_t * rtsp,
                              const char * command,
                              const char * arg,
                              char ** fields,
                              int milliseconds)
  {
  char * line;
  const char * var;
  int i;
  line = bgav_sprintf("%s %s RTSP/1.0\r\n", command, arg);
  fprintf(stderr, "Sending line: %s", line);
  write(rtsp->fd, line, strlen(line));
  free(line);
  
  line = bgav_sprintf("CSeq: %u\r\n", rtsp->cseq);
  fprintf(stderr, "Sending line: %s", line);
  write(rtsp->fd, line, strlen(line));
  free(line);

  if(rtsp->session)
    {
    line = bgav_sprintf("Session: %s\r\n", rtsp->session);
    fprintf(stderr, "Sending line: %s", line);
    write(rtsp->fd, line, strlen(line));
    free(line);
    }
  
  if(fields)
    {
    i = 0;
    while(fields[i])
      {
      fprintf(stderr, "Sending line: %s\n", fields[i]);
      write(rtsp->fd, fields[i], strlen(fields[i]));
      write(rtsp->fd, "\r\n", 2);
      i++;
      }
    }
  write(rtsp->fd, "\r\n", 2);
  fprintf(stderr, "Request done\n");

  /* Read answers */
  bgav_http_header_reset(rtsp->answers);
  bgav_http_header_revc(rtsp->answers, rtsp->fd, milliseconds);
  bgav_http_header_dump(rtsp->answers);

  var = bgav_http_header_get_var(rtsp->answers, "Session");
  if(var && !(rtsp->session)) 
    rtsp->session = bgav_strndup(var, NULL);
  }

char * init_fields[] =
  {
    "User-Agent: RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)",
    "ClientChallenge: 9e26d33f2984236010ef6253fb1887f7",
    "PlayerStarttime: [28/03/2003:22:50:23 00:00]",
    "CompanyID: KnKV4M4I/B2FjJ1TToLycw==",
    "GUID: 00000000-0000-0000-0000-000000000000",
    "RegionData: 0",
    "ClientID: Linux_2.4_6.0.9.1235_play32_RN01_EN_586",
    "Pragma: initiate-session",
    (char*)0
  };

char * describe_fields[] =
  {
    "Accept: application/sdp",
    "Bandwidth: 10485800",
    "GUID: 00000000-0000-0000-0000-000000000000",
    "RegionData: 0",
    "ClientID: Linux_2.4_6.0.9.1235_play32_RN01_EN_586",
    "SupportsMaximumASMBandwidth: 1",
    "Language: en-US",
    "Require: com.real.retain-entity-for-setup",
    (char*)0
  };

/*
 *  Open connection to the server, get options and handle redirections
 *
 *  Return FALSE if error, TRUE on success.
 *  If we got redirectred, TRUE is returned and got_redirected
 *  is set to 1. The new URL is copied to the rtsp structure
 */

static int do_connect(bgav_rtsp_t * rtsp,
                      int * got_redirected, int milliseconds)
  {
  int port = -1;
  char * host = (char*)0;
  char * protocol = (char*)0;
  char ** strings = (char**)0;
  int ret = 0;
  if(!bgav_url_split(rtsp->url, &protocol, &host,
                     &port, (char**)0))
    goto done;
  
  if(strcmp(protocol, "rtsp"))
    goto done;

  if(port == -1)
    port = 554;

  rtsp->cseq = 1;
  rtsp->fd = bgav_tcp_connect(host, port, milliseconds);
  if(rtsp->fd < 0)
    goto done;

  rtsp_send_request(rtsp, "OPTIONS", rtsp->url, init_fields, milliseconds);
  
  /* Get the server status */
  
  strings = bgav_stringbreak(rtsp->answers->lines[0].line, ' ');
  if(!strings)
    goto done;
  
  if(!strncmp(strings[0], "REDIRECT", 8))
    {
    free(rtsp->url);
    rtsp->url =
      bgav_strndup(bgav_http_header_get_var(rtsp->answers,"Location"),
                   NULL);
    *got_redirected = 1;
    ret = 1;
    goto done;
    }
  ret = 1;

  done:

  if(strings)
    bgav_stringbreak_free(strings);
  
  if(host)
    free(host);
  if(protocol)
    free(protocol);
  return ret;
  }

bgav_rtsp_t *
bgav_rtsp_open(const char * url, int milliseconds,
               const char * user_agent)
  {
  //  char * command;
  bgav_rtsp_t * ret = (bgav_rtsp_t*)0;
  const char * var;
  int got_redirected;
  char * buf = (char*)0;
  int content_length;
  
  ret = calloc(1, sizeof(*ret));
  ret->url = bgav_strndup(url, NULL);
  ret->user_agent = bgav_sprintf(user_agent);
  ret->answers = bgav_http_header_create();
  got_redirected = 0;
  do{
    if(!do_connect(ret, &got_redirected, milliseconds))
      goto fail;
  }while(got_redirected);
  
  /* Send the "DESCRIBE" request */
  
  ret->cseq++;
  rtsp_send_request(ret, "DESCRIBE", url, describe_fields, milliseconds);

  var = bgav_http_header_get_var(ret->answers, "Content-length");

  if(!var)
    goto fail;

  content_length = atoi(var);
    
  fprintf(stderr, "Content-length: %d\n", content_length);
  
  buf = malloc(content_length+1);
    
  if(bgav_read_data_fd(ret->fd, buf, content_length, milliseconds) <
     content_length)
    {
    goto fail;
    }

  buf[content_length] = '\0';
  fprintf(stderr, "Session description: ");
  fwrite(buf, 1, content_length, stderr);
  
  if(!bgav_sdp_parse(buf, &(ret->sdp)))
    goto fail;

  fprintf(stderr, "SDP:\n");
  bgav_sdp_dump(&(ret->sdp));

  free(buf);
  
  return ret;
  
  fail:
  
  if(buf)
    free(buf);
  if(ret)
    free(ret);
  return (bgav_rtsp_t*)0;
  }

void bgav_rtsp_close(bgav_rtsp_t * r)
  {
  
  }

