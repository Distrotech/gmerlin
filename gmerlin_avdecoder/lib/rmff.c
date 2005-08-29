/*****************************************************************
 
  rmff.c
 
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

#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <stdio.h>
#include <rmff.h>
#include <asmrp.h>

#define FILE_ID BGAV_MK_FOURCC('.', 'R', 'M', 'F')
#define PROP_ID BGAV_MK_FOURCC('P', 'R', 'O', 'P')
#define MDPR_ID BGAV_MK_FOURCC('M', 'D', 'P', 'R')
#define CONT_ID BGAV_MK_FOURCC('C', 'O', 'N', 'T')
#define DATA_ID BGAV_MK_FOURCC('D', 'A', 'T', 'A')
#define INDX_ID BGAV_MK_FOURCC('I', 'N', 'D', 'X')

static char * read_data(bgav_input_context_t * input, int len)
  {
  char * ret;
  ret = malloc(len+1);
  if(bgav_input_read_data(input, (uint8_t*)ret, len) < len)
    {
    free(ret);
    return (char*)0;
    }
  ret[len] = '\0';
  return ret;
  }

static void dump_string(const char * str, int len)
  {
  int  i;
  for(i = 0; i < len; i++)
    fputc(str[i], stderr);
  }

/* File header */

int bgav_rmff_file_header_read(bgav_rmff_chunk_t * c,
                               bgav_input_context_t * input,
                               bgav_rmff_file_header_t * ret)
  {
  return bgav_input_read_32_be(input, &(ret->file_version)) &&
    bgav_input_read_32_be(input, &(ret->num_headers));
  }


/* Chunk header */

int bgav_rmff_chunk_header_read(bgav_rmff_chunk_t * c, bgav_input_context_t * input)
  {
  return bgav_input_read_fourcc(input, &(c->id)) &&
    bgav_input_read_32_be(input, &(c->size))&&
    bgav_input_read_16_be(input, &(c->version));
  }


/* PROP */

void bgav_rmff_prop_dump(bgav_rmff_prop_t * p)
  {
  fprintf(stderr, "PROP:\n");
  
  fprintf(stderr, "  max_bit_rate:    %d\n", p->max_bit_rate);
  fprintf(stderr, "  avg_bit_rate:    %d\n", p->avg_bit_rate);
  fprintf(stderr, "  max_packet_size: %d\n", p->max_packet_size);
  fprintf(stderr, "  avg_packet_size: %d\n", p->avg_packet_size);
  fprintf(stderr, "  num_packets:     %d\n", p->num_packets);
  fprintf(stderr, "  duration:        %d\n", p->duration);
  fprintf(stderr, "  preroll:         %d\n", p->preroll);
  fprintf(stderr, "  index_offset:    %d\n", p->index_offset);
  fprintf(stderr, "  data_offset:     %d\n", p->data_offset);
  fprintf(stderr, "  num_streams:     %d\n", p->num_streams);
  fprintf(stderr, "  flags:           %d\n", p->flags);
  }

int bgav_rmff_prop_read(bgav_rmff_chunk_t * c,
                        bgav_input_context_t * input,
                        bgav_rmff_prop_t * ret)
  {
  int result;
  result =
    bgav_input_read_32_be(input, &(ret->max_bit_rate)) &&
    bgav_input_read_32_be(input, &(ret->avg_bit_rate)) &&
    bgav_input_read_32_be(input, &(ret->max_packet_size)) &&
    bgav_input_read_32_be(input, &(ret->avg_packet_size)) &&
    bgav_input_read_32_be(input, &(ret->num_packets)) &&
    bgav_input_read_32_be(input, &(ret->duration)) &&
    bgav_input_read_32_be(input, &(ret->preroll)) &&
    bgav_input_read_32_be(input, &(ret->index_offset)) &&
    bgav_input_read_32_be(input, &(ret->data_offset)) &&
    bgav_input_read_16_be(input, &(ret->num_streams)) &&
    bgav_input_read_16_be(input, &(ret->flags));
  //  dump_prop(ret);
  return result;
  }

/* MDPR */

void bgav_rmff_mdpr_dump(bgav_rmff_mdpr_t * m)
  {
  fprintf(stderr, "MDPR:\n");
  fprintf(stderr, "  stream_number:    %d\n", m->stream_number);
  fprintf(stderr, "  max_bit_rate:     %d\n", m->max_bit_rate);
  fprintf(stderr, "  avg_bit_rate:     %d\n", m->avg_bit_rate);
  fprintf(stderr, "  max_packet_size:  %d\n", m->max_packet_size);
  fprintf(stderr, "  avg_packet_size:  %d\n", m->avg_packet_size);
  fprintf(stderr, "  start_time:       %d\n", m->start_time);
  fprintf(stderr, "  preroll:          %d\n", m->preroll);
  fprintf(stderr, "  duration:         %d\n", m->duration);
  fprintf(stderr, "  stream_name:      ");
  dump_string(m->stream_name, m->stream_name_size);
  fprintf(stderr, "\n  mime_type:        ");
  dump_string(m->mime_type, m->mime_type_size);
  fprintf(stderr, "\n  type_specific_len:  %d", m->type_specific_len);
  fprintf(stderr, "\n  type_specific_data:\n");
  bgav_hexdump(m->type_specific_data, m->type_specific_len, 16);
  }

int bgav_rmff_mdpr_read(bgav_rmff_chunk_t * c,
                        bgav_input_context_t * input,
                        bgav_rmff_mdpr_t * ret)
  {
  if(!bgav_input_read_16_be(input, &(ret->stream_number)) ||
     !bgav_input_read_32_be(input, &(ret->max_bit_rate)) ||
     !bgav_input_read_32_be(input, &(ret->avg_bit_rate)) ||
     !bgav_input_read_32_be(input, &(ret->max_packet_size)) ||
     !bgav_input_read_32_be(input, &(ret->avg_packet_size)) ||
     !bgav_input_read_32_be(input, &(ret->start_time)) ||
     !bgav_input_read_32_be(input, &(ret->preroll)) ||
     !bgav_input_read_32_be(input, &(ret->duration)) ||
     !bgav_input_read_8(input, &(ret->stream_name_size)))
    return 0;

  ret->stream_name = read_data(input, ret->stream_name_size);
  if(!ret->stream_name)
    return 0;

  if(!bgav_input_read_8(input, &(ret->mime_type_size)))
    return 0;

  ret->mime_type = read_data(input, ret->mime_type_size);
  if(!ret->mime_type)
    return 0;

  if(!bgav_input_read_32_be(input, &(ret->type_specific_len)))
    return 0;

  if(!ret->type_specific_len)
    return 1;
  
  ret->type_specific_data = (uint8_t*)read_data(input, ret->type_specific_len);
  if(!ret->type_specific_data)
    return 0;
  return 1;
  }

void bgav_rmff_mdpr_destroy(bgav_rmff_mdpr_t * m)
  {
  if(m->stream_name)
    free(m->stream_name);
  if(m->mime_type)
    free(m->mime_type);
  if(m->type_specific_data)
    free(m->type_specific_data);
  }

/* Content description */

/*
 *  The metadata are ASCII format, so we can read them
 *  here as UTF-8
 */

#define READ_STRING(dst, dst_len)                 \
  if(!bgav_input_read_16_be(input, &(dst_len)))   \
    return 0;\
  if(dst_len) \
    { \
    ret->dst = malloc(dst_len); \
    if(bgav_input_read_data(input, (uint8_t*)(ret->dst), dst_len) < dst_len) \
      { \
      free(ret->dst); \
      ret->dst = (char*)0; \
      return 0; \
      } \
    }

int bgav_rmff_cont_read(bgav_rmff_chunk_t * c,
                        bgav_input_context_t * input,
                        bgav_rmff_cont_t * ret)
  {
  READ_STRING(title, ret->title_len);
  READ_STRING(author, ret->author_len);
  READ_STRING(copyright, ret->copyright_len);
  READ_STRING(comment, ret->comment_len);
  return 1;
  }

void bgav_rmff_cont_dump(bgav_rmff_cont_t * cont)
  {
  fprintf(stderr, "CONT:");
  fprintf(stderr, "\n  Title:     ");
  dump_string(cont->title, cont->title_len);
  fprintf(stderr, "\n  Author:    ");
  dump_string(cont->author, cont->author_len);
  fprintf(stderr, "\n  Copyright: ");
  dump_string(cont->copyright, cont->copyright_len);
  fprintf(stderr, "\n  Comment:   ");
  dump_string(cont->comment, cont->comment_len);
  fprintf(stderr, "\n");
  }

#define MY_FREE(ptr) if(ptr)free(ptr)

void bgav_rmff_cont_free(bgav_rmff_cont_t * c)
  {
  MY_FREE(c->title);
  MY_FREE(c->author);
  MY_FREE(c->copyright);
  MY_FREE(c->comment);
  }

/* Index stuff */

int bgav_rmff_index_record_read(bgav_input_context_t * input,
                                bgav_rmff_index_record_t * ret)
  {
  return
    bgav_input_read_16_be(input, &(ret->version)) &&
    bgav_input_read_32_be(input, &(ret->timestamp)) &&
    bgav_input_read_32_be(input, &(ret->offset)) &&
    bgav_input_read_32_be(input, &(ret->packet_count_for_this_packet));
  }

int bgav_rmff_indx_read(bgav_input_context_t * input,
                        bgav_rmff_indx_t * ret)
  {
  int i;
  if(!bgav_input_read_32_be(input, &(ret->num_indices)) ||
     !bgav_input_read_16_be(input, &(ret->stream_number)) ||
     !bgav_input_read_32_be(input, &(ret->next_index_header)))
    goto fail;
  if(ret->num_indices)
    {
    ret->records = malloc(ret->num_indices * sizeof(*(ret->records)));
    for(i = 0; i < ret->num_indices; i++)
      {
      if(!bgav_rmff_index_record_read(input, &(ret->records[i])))
        goto fail;
      }
    }
  return 1;
  fail:
  if(ret->records)
    free(ret->records);
  memset(ret, 0, sizeof(*ret));
  return 0;
  }

void bgav_rmff_indx_free(bgav_rmff_indx_t * ret)
  {
  if(ret->records)
    free(ret->records);
  memset(ret, 0, sizeof(*ret));
  }

void bgav_rmff_indx_dump(bgav_rmff_indx_t * indx)
  {
  int i;
  fprintf(stderr, "num_indices:       %d",   indx->num_indices);
  fprintf(stderr, "stream_number:     %d\n", indx->stream_number);
  fprintf(stderr, "next_index_header: %d\n", indx->next_index_header);
  /*
    uint32_t timestamp;
    uint32_t offset;
    uint32_t packet_count_for_this_packet;
  */

  for(i = 0; i < indx->num_indices; i++)
    {
    fprintf(stderr, "Time: %d, offset: %d, count: %d\n",
            indx->records[i].timestamp,
            indx->records[i].offset,
            indx->records[i].packet_count_for_this_packet);
    }
  }

/* Data header */

int bgav_rmff_data_header_read(bgav_input_context_t * input,
                               bgav_rmff_data_header_t * data)
  {
  return bgav_input_read_32_be(input, &(data->num_packets)) &&
    bgav_input_read_32_be(input, &(data->next_data_header));
  }


/* Overall file header */

void bgav_rmff_header_dump(bgav_rmff_header_t * header)
  {
  int i;
  fprintf(stderr, "rmff_header:\n");
  bgav_rmff_prop_dump(&(header->prop));
  bgav_rmff_cont_dump(&(header->cont));

  fprintf(stderr, "Number of streams: %d\n", header->num_streams);
  
  for(i = 0; i < header->num_streams; i++)
    {
    bgav_rmff_mdpr_dump(&(header->streams[i].mdpr));
    }
  }

bgav_rmff_header_t * bgav_rmff_header_read(bgav_input_context_t * ctx)
  {
  int i;
  bgav_rmff_indx_t indx;
  bgav_rmff_chunk_t chunk;
  int keep_going = 1;
  uint32_t next_index_header;
  
  bgav_rmff_header_t * ret = calloc(1, sizeof(*ret));

  if(!bgav_rmff_chunk_header_read(&chunk, ctx))
    return 0;

  if(chunk.id != FILE_ID)
    return 0;

  if(!bgav_rmff_file_header_read(&chunk, ctx, &(ret->file_header)))
    return 0;

  while(keep_going)
    {
    if(!bgav_rmff_chunk_header_read(&chunk, ctx))
      return 0;
    switch(chunk.id)
      {
      case PROP_ID:
        if(!bgav_rmff_prop_read(&chunk,ctx, &(ret->prop)))
          goto fail;
        break;
      case CONT_ID:
        if(!bgav_rmff_cont_read(&chunk,ctx,&(ret->cont)))
          goto fail;
        break;
      case MDPR_ID:
        ret->streams = realloc(ret->streams, (ret->num_streams+1) *
                               sizeof(*(ret->streams)));

        memset(ret->streams + ret->num_streams, 0, sizeof(*(ret->streams)));
        
        if(!bgav_rmff_mdpr_read(&chunk,ctx,&(ret->streams[ret->num_streams].mdpr)))
          goto fail;
        ret->num_streams++;
        break;
      case DATA_ID:
        //        fprintf(stderr, "Data\n");
        
        /* Read data chunk header */
        if(!bgav_rmff_data_header_read(ctx, &(ret->data_header)))
          goto fail;
        
        /* We reached the data section. Now check if the file is seekabkle
           and read the indices */
        ret->data_start = ctx->position;
        if(chunk.size > 10)
          ret->data_size = chunk.size - 10;
        else if(ret->prop.index_offset)
          ret->data_size = ret->prop.index_offset - ret->data_start;
        else
          ret->data_size = 0;
        if(ctx->input->seek_byte && ret->prop.index_offset)
          {
          bgav_input_seek(ctx, ret->prop.index_offset, SEEK_SET);
          
          while(1)
            {
            bgav_rmff_chunk_header_read(&chunk, ctx);
            //            fprintf(stderr, "Index fourcc: ");
            if(chunk.id != INDX_ID)
              {
              fprintf(stderr, "No index found, where I expected one\n");
              break;
              }
            if(!bgav_rmff_indx_read(ctx, &indx))
              {
              keep_going = 0;
              break;
              }
            next_index_header = indx.next_index_header;
            
            //            dump_indx(&indx);
            
            for(i = 0; i < ret->num_streams; i++)
              {
              if(ret->streams[i].mdpr.stream_number == indx.stream_number)
                {
                memcpy(&(ret->streams[i].indx), &indx, sizeof(indx));
                memset(&indx, 0, sizeof(indx));
                ret->streams[i].has_indx = 1;
                break;
                }
              }
            
            bgav_rmff_indx_free(&indx);

            if(next_index_header)
              {
              bgav_input_skip(ctx,
                              next_index_header - ctx->position);
              //              fprintf(stderr, "Skipping %lld bytes",
              //                      next_index_header - ctx->input->position);
              }
            else
              break;
            }
          bgav_input_seek(ctx, ret->data_start, SEEK_SET);
          }
        keep_going = 0;
        //        bgav_input_skip(ctx->input, chunk.size - 10);
        break;
      case INDX_ID:
        //        fprintf(stderr, "Index\n");
        bgav_input_skip(ctx, chunk.size - 10);
        break;
      }
    }
  //  bgav_rmff_header_dump(ret);
  return ret;
  fail:
  bgav_rmff_header_destroy(ret);
  return (bgav_rmff_header_t *)0;
  }

bgav_rmff_header_t * bgav_rmff_header_create(int num_streams)
  {
  bgav_rmff_header_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->num_streams = num_streams;
  ret->prop.num_streams = num_streams;
  ret->streams = calloc(ret->num_streams, sizeof(*(ret->streams)));
  return ret;
  }

void bgav_rmff_header_destroy(bgav_rmff_header_t * h)
  {
  int i;
  if(h->streams)
    {
    for(i = 0; i < h->num_streams; i++)
      {
      bgav_rmff_indx_free(&(h->streams[i].indx));
      bgav_rmff_mdpr_destroy(&(h->streams[i].mdpr));
      }
    free(h->streams);
    }
  bgav_rmff_cont_free(&(h->cont));
  free(h);
  }

/* Ripped from mplayer */

/*
 * takes a MLTI-Chunk and a rule number got from match_asm_rule,
 * returns a pointer to selected data and number of bytes in that.
 */

static int select_mlti_data(const uint8_t *mlti_chunk, int mlti_size, int selection, uint8_t **out)
  {
  int numrules, codec, size;
  int i;
  
  /* MLTI chunk should begin with MLTI */

  if ((mlti_chunk[0] != 'M')
      ||(mlti_chunk[1] != 'L')
      ||(mlti_chunk[2] != 'T')
      ||(mlti_chunk[3] != 'I'))
  {
  *out = malloc(mlti_size);
  memcpy(*out, mlti_chunk, mlti_size);
  return mlti_size;
  }

  mlti_chunk+=4;

  /* next 16 bits are the number of rules */
  numrules=BGAV_PTR_2_16BE(mlti_chunk);
  if (selection >= numrules) return 0;

  /* now <numrules> indices of codecs follows */
  /* we skip to selection                     */
  mlti_chunk+=(selection+1)*2;

  /* get our index */
  codec=BGAV_PTR_2_16BE(mlti_chunk);

  /* skip to number of codecs */
  mlti_chunk+=(numrules-selection)*2;

  /* get number of codecs */
  numrules=BGAV_PTR_2_16BE(mlti_chunk);

  if (codec >= numrules) {
    printf("codec index >= number of codecs. %i %i\n", codec, numrules);
    return 0;
  }

  mlti_chunk+=2;
 
  /* now seek to selected codec */
  for (i=0; i<codec; i++) {
    size=BGAV_PTR_2_32BE(mlti_chunk);
    mlti_chunk+=size+4;
  }
  
  size=BGAV_PTR_2_32BE(mlti_chunk);

  *out = malloc(size);
  memcpy(*out, mlti_chunk+4, size);
  return size;
}




#define GET_ATTR_INT(attrs, num_attrs, name, dst) \
  if(bgav_sdp_get_attr_int(attrs, num_attrs, name, &i_tmp)) \
    dst = i_tmp; \
  else \
    fprintf(stderr, "Attribute "name" not there\n")

#define GET_ATTR_DATA(attrs, num_attrs, name, dst, dst_len)  \
  if(bgav_sdp_get_attr_data(attrs, num_attrs, name, &buffer, &(i_tmp)) \
     && i_tmp)       \
    { \
    dst_len = i_tmp; \
    dst = malloc(dst_len); \
    memcpy(dst, buffer, dst_len); \
    } \
  else \
    fprintf(stderr, "Attribute "name" not there\n")

#define GET_ATTR_STRING(attrs, num_attrs, name, dst, dst_len)   \
  if(bgav_sdp_get_attr_string(attrs, num_attrs, name, &str))    \
    {                                                           \
    dst = bgav_strndup(str, NULL);                              \
    dst_len = strlen(dst);                                      \
    }                                                           \
  else                                                          \
    fprintf(stderr, "Attribute "name" not there\n")

/* Create a real media header from an sdp object */

bgav_rmff_header_t *
bgav_rmff_header_create_from_sdp(bgav_sdp_t * sdp, int network_bandwidth, char ** stream_rules)
  {
  char * buf;
  char * pos;
  int i, j;
  int i_tmp;
  uint8_t * buffer;
  char * str;
  bgav_rmff_header_t * ret;
  char * asm_rulebook;
  int matches[16];
  int num_matches;

  uint8_t * opaque_data;
  int opaque_data_len;
    
  //  bgav_sdp_dump(sdp);

  /* Set up global stuff */

  ret = bgav_rmff_header_create(sdp->num_media);
  
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "MaxBitRate", ret->prop.max_bit_rate );
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "AvgBitRate", ret->prop.avg_bit_rate);
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "MaxPacketSize", ret->prop.max_packet_size );
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "AvgPacketSize", ret->prop.avg_packet_size );
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "Preroll", ret->prop.preroll  );
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "IndexOffset", ret->prop.index_offset );
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "DataOffset", ret->prop.data_offset );
  GET_ATTR_INT(sdp->attributes, sdp->num_attributes,
               "Flags", ret->prop.flags );

  GET_ATTR_DATA(sdp->attributes, sdp->num_attributes,
                "Author", ret->cont.author, ret->cont.author_len);
  GET_ATTR_DATA(sdp->attributes, sdp->num_attributes,
                "Title", ret->cont.title, ret->cont.title_len);
  GET_ATTR_DATA(sdp->attributes, sdp->num_attributes,
                "Copyright", ret->cont.copyright, ret->cont.copyright_len);
  GET_ATTR_DATA(sdp->attributes, sdp->num_attributes,
                "Comment", ret->cont.comment, ret->cont.comment_len);
  
  /* Set up streams */

  for(i = 0; i < sdp->num_media; i++)
    {
    GET_ATTR_INT(sdp->media[i].attributes, sdp->media[i].num_attributes,
                 "MaxBitRate", ret->streams[i].mdpr.max_bit_rate );
    GET_ATTR_INT(sdp->media[i].attributes, sdp->media[i].num_attributes,
                 "AvgBitRate", ret->streams[i].mdpr.avg_bit_rate);
    GET_ATTR_INT(sdp->media[i].attributes, sdp->media[i].num_attributes,
                 "MaxPacketSize", ret->streams[i].mdpr.max_packet_size );
    GET_ATTR_INT(sdp->media[i].attributes, sdp->media[i].num_attributes,
                 "AvgPacketSize", ret->streams[i].mdpr.avg_packet_size );
    GET_ATTR_INT(sdp->media[i].attributes, sdp->media[i].num_attributes,
                 "Preroll", ret->streams[i].mdpr.preroll );

    GET_ATTR_STRING(sdp->media[i].attributes, sdp->media[i].num_attributes,
                    "StreamName", ret->streams[i].mdpr.stream_name,
                    ret->streams[i].mdpr.stream_name_size);
    GET_ATTR_STRING(sdp->media[i].attributes, sdp->media[i].num_attributes,
                    "mimetype", ret->streams[i].mdpr.mime_type,
                    ret->streams[i].mdpr.mime_type_size);

    /* Get stream ID */
    if(!bgav_sdp_get_attr_string(sdp->media[i].attributes, sdp->media[i].num_attributes,
                                 "control", &str))
      {
      fprintf(stderr, "Control attribute missing\n");
      goto fail;
      }

    if(!(pos = strstr(str, "streamid=")))
      {
      fprintf(stderr, "Stream number missing\n");
      goto fail;
      }
    pos += 9;
    ret->streams[i].mdpr.stream_number = atoi(pos);
    
    if(!bgav_sdp_get_attr_string(sdp->media[i].attributes, sdp->media[i].num_attributes,
                                 "ASMRuleBook", &asm_rulebook))
      {
      fprintf(stderr, "No ASMRuleBook found\n");
      goto fail;
      }

    num_matches = bgav_asmrp_match(asm_rulebook, network_bandwidth, matches);

    if(!num_matches)
      {
      fprintf(stderr, "Bad ASMRuleBook\n");
      goto fail;
      }
    for(j = 0; j < num_matches; j++)
      {
      buf = bgav_sprintf("stream=%u;rule=%u,", ret->streams[i].mdpr.stream_number, matches[j]);
      *stream_rules = bgav_strncat(*stream_rules, buf, NULL);
      free(buf);
      }

    if(!bgav_sdp_get_attr_data(sdp->media[i].attributes, sdp->media[i].num_attributes,
                               "OpaqueData", &opaque_data, &opaque_data_len))
      {
      fprintf(stderr, "No Opaque data there\n");
      goto fail;
      }

    ret->streams[i].mdpr.type_specific_len =
      select_mlti_data(opaque_data, opaque_data_len, matches[0],
                       &(ret->streams[i].mdpr.type_specific_data));
    
    }

    
  //  bgav_rmff_header_dump(ret);
  
  return ret;

  fail:
  bgav_rmff_header_destroy(ret);
  return (bgav_rmff_header_t*)0;
  }

int bgav_rmff_packet_header_read(bgav_input_context_t * input,
                                 bgav_rmff_packet_header_t * ret)
  {
  return
    bgav_input_read_16_be(input, &(ret->object_version)) &&
    bgav_input_read_16_be(input, &(ret->length)) &&
    bgav_input_read_16_be(input, &(ret->stream_number)) &&
    bgav_input_read_32_be(input, &(ret->timestamp)) &&
    bgav_input_read_8(input, &(ret->reserved)) &&
    bgav_input_read_8(input, &(ret->flags));
  }

void bgav_rmff_packet_header_dump(bgav_rmff_packet_header_t * h)
  {
  fprintf(stderr, "Packet L: %d, S: %d, T: %d, F: %x\n",
          h->length, h->stream_number, h->timestamp, h->flags);
  }

void bgav_rmff_packet_header_to_pointer(bgav_rmff_packet_header_t * h,
                                        uint8_t * ptr)
  {
  BGAV_16BE_2_PTR(h->object_version, ptr);ptr+=2;
  BGAV_16BE_2_PTR(h->length, ptr);ptr+=2;
  BGAV_16BE_2_PTR(h->stream_number, ptr);ptr+=2;
  BGAV_32BE_2_PTR(h->timestamp, ptr);ptr+=4;
  *ptr = h->reserved;ptr++;
  *ptr = h->flags;ptr++;
  }
