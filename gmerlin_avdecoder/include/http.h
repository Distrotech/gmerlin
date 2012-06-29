/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

/* http support */

typedef struct
  {
  int num_lines;
  int lines_alloc;
  struct
    {
    char * line;    /* 0 terminated */
    int line_alloc; /* Allocated size */
    } * lines;
  } bgav_http_header_t;

typedef struct bgav_http_s bgav_http_t;

/* Creation/destruction of http header */

bgav_http_header_t * bgav_http_header_create();

void bgav_http_header_reset(bgav_http_header_t*);
void bgav_http_header_destroy(bgav_http_header_t*);

void bgav_http_header_add_line(bgav_http_header_t*, const char * line);
int bgav_http_header_send(const bgav_options_t * opt,
                          bgav_http_header_t*, int fd);

int bgav_http_header_status_code(bgav_http_header_t * h);
const char * bgav_http_header_status_line(bgav_http_header_t * h);

/* Reading of http header */

int bgav_http_header_revc(const bgav_options_t * opt,
                          bgav_http_header_t*, int fd);

const char * bgav_http_header_get_var(bgav_http_header_t*,
                                      const char * name);


void bgav_http_header_dump(bgav_http_header_t*);

/* http connection */

bgav_http_t * bgav_http_open(const char * url,
                             const bgav_options_t * opt,
                             char ** redirect_url,
                             bgav_http_header_t* extra_header);

void bgav_http_close(bgav_http_t *);

int bgav_http_get_fd(bgav_http_t *);

bgav_http_header_t* bgav_http_get_header(bgav_http_t *);
