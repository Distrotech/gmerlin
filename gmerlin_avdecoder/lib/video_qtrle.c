/*
    Quicktime Animation (RLE) Decoder for MPlayer

    (C) 2001 Mike Melanson
    8 and 16bpp support by Alex Beregszaszi
    32 bpp support by Roberto Togni
*/
#include <stdlib.h>

// #include "config.h"
#include <bswap.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

#define BE_16(x) (be2me_16(*(unsigned short *)(x)))
#define BE_32(x) (be2me_32(*(unsigned int *)(x)))
#if 0
static void qt_decode_rle8(
  unsigned char *encoded,
  int encoded_size,
  unsigned char *decoded,
  int width,
  int height,
  int bytes_per_pixel)
{
  int stream_ptr;
  int header;
  int start_line;
  int lines_to_change;
  signed char rle_code;
  int row_ptr, pixel_ptr;
  int row_inc = bytes_per_pixel * width;
  unsigned char pixel;

  // check if this frame is even supposed to change
  if (encoded_size < 8)
    return;

  // start after the chunk size
  stream_ptr = 4;

  // fetch the header
  header = BE_16(&encoded[stream_ptr]);
  stream_ptr += 2;

  // if a header is present, fetch additional decoding parameters
  if (header & 0x0008)
  {
    start_line = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
    lines_to_change = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
  }
  else
  {
    start_line = 0;
    lines_to_change = height;
  }

  row_ptr = row_inc * start_line;
  while (lines_to_change--)
  {
    pixel_ptr = row_ptr + ((encoded[stream_ptr++] - 1) * bytes_per_pixel);

    while (stream_ptr < encoded_size &&
           (rle_code = (signed char)encoded[stream_ptr++]) != -1)
    {
      if (rle_code == 0)
        // there's another skip code in the stream
        pixel_ptr += ((encoded[stream_ptr++] - 1) * bytes_per_pixel);
      else if (rle_code < 0)
      {
        // decode the run length code
        rle_code = -rle_code;
        pixel = encoded[stream_ptr++];
        while (rle_code--)
        {
          decoded[pixel_ptr++] = pixel;
        }
      }
      else
      {
        // copy pixels directly to output
        while (rle_code--)
        {
          decoded[pixel_ptr++] = encoded[stream_ptr + 0];
          stream_ptr += 1;
        }
      }
    }

    row_ptr += row_inc;
  }
}
#endif
static void qt_decode_rle16(
  unsigned char *encoded,
  int encoded_size,
  unsigned char *_decoded,
  int width,
  int height,
  int bytes_per_pixel)
{
  int stream_ptr;
  int header;
  int start_line;
  int lines_to_change;
  signed char rle_code;
  int row_ptr, pixel_ptr;
  int row_inc = width;
  unsigned char p1, p2;
  uint16_t * decoded = (uint16_t*)_decoded;
  // check if this frame is even supposed to change
  if (encoded_size < 8)
    return;

  // start after the chunk size
  stream_ptr = 4;

  // fetch the header
  header = BE_16(&encoded[stream_ptr]);
  stream_ptr += 2;

  // if a header is present, fetch additional decoding parameters
  if (header & 0x0008)
  {
    start_line = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
    lines_to_change = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
  }
  else
  {
    start_line = 0;
    lines_to_change = height;
  }

  row_ptr = row_inc * start_line;
  while (lines_to_change--)
  {
    pixel_ptr = row_ptr + (encoded[stream_ptr++] - 1);

    while (stream_ptr < encoded_size &&
           (rle_code = (signed char)encoded[stream_ptr++]) != -1)
    {
      if (rle_code == 0)
        // there's another skip code in the stream
        pixel_ptr += ((encoded[stream_ptr++] - 1) * bytes_per_pixel);
      else if (rle_code < 0)
      {
        // decode the run length code
        rle_code = -rle_code;
        p1 = encoded[stream_ptr++];
        p2 = encoded[stream_ptr++];
        while (rle_code--)
        {
          decoded[pixel_ptr] = (p1<<8) | p2;
          pixel_ptr++;
        }
      }
      else
      {
        // copy pixels directly to output
        while (rle_code--)
        {
          p1 = encoded[stream_ptr++];
          p2 = encoded[stream_ptr++];
          decoded[pixel_ptr] = (p1<<8) | p2;
          stream_ptr += 2;
          pixel_ptr++;
        }
      }
    }

    row_ptr += row_inc;
  }
}

static void qt_decode_rle24(
  unsigned char *encoded,
  int encoded_size,
  unsigned char *decoded,
  int width,
  int height,
  int bytes_per_pixel)
{
  int stream_ptr;
  int header;
  int start_line;
  int lines_to_change;
  signed char rle_code;
  int row_ptr, pixel_ptr;
  int row_inc = bytes_per_pixel * width;
  unsigned char r, g, b;

  // check if this frame is even supposed to change
  if (encoded_size < 8)
    return;

  // start after the chunk size
  stream_ptr = 4;

  // fetch the header
  header = BE_16(&encoded[stream_ptr]);
  stream_ptr += 2;

  // if a header is present, fetch additional decoding parameters
  if (header & 0x0008)
  {
    start_line = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
    lines_to_change = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
  }
  else
  {
    start_line = 0;
    lines_to_change = height;
  }

  row_ptr = row_inc * start_line;
  while (lines_to_change--)
  {
    pixel_ptr = row_ptr + ((encoded[stream_ptr++] - 1) * bytes_per_pixel);

    while (stream_ptr < encoded_size &&
           (rle_code = (signed char)encoded[stream_ptr++]) != -1)
    {
      if (rle_code == 0)
        // there's another skip code in the stream
        pixel_ptr += ((encoded[stream_ptr++] - 1) * bytes_per_pixel);
      else if (rle_code < 0)
      {
        // decode the run length code
        rle_code = -rle_code;
        r = encoded[stream_ptr++];
        g = encoded[stream_ptr++];
        b = encoded[stream_ptr++];
        while (rle_code--)
        {
          decoded[pixel_ptr++] = b;
          decoded[pixel_ptr++] = g;
          decoded[pixel_ptr++] = r;
          if (bytes_per_pixel == 4)
            pixel_ptr++;
        }
      }
      else
      {
        // copy pixels directly to output
        while (rle_code--)
        {
          decoded[pixel_ptr++] = encoded[stream_ptr + 2];
          decoded[pixel_ptr++] = encoded[stream_ptr + 1];
          decoded[pixel_ptr++] = encoded[stream_ptr + 0];
          stream_ptr += 3;
          if (bytes_per_pixel == 4)
            pixel_ptr++;
        }
      }
    }

    row_ptr += row_inc;
  }
}

static void qt_decode_rle32(
  unsigned char *encoded,
  int encoded_size,
  unsigned char *decoded,
  int width,
  int height,
  int bytes_per_pixel)
{
  int stream_ptr;
  int header;
  int start_line;
  int lines_to_change;
  signed char rle_code;
  int row_ptr, pixel_ptr;
  int row_inc = bytes_per_pixel * width;
  unsigned char r, g, b;

  // check if this frame is even supposed to change
  if (encoded_size < 8)
    return;

  // start after the chunk size
  stream_ptr = 4;

  // fetch the header
  header = BE_16(&encoded[stream_ptr]);
  stream_ptr += 2;

  // if a header is present, fetch additional decoding parameters
  if (header & 0x0008)
  {
    start_line = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
    lines_to_change = BE_16(&encoded[stream_ptr]);
    stream_ptr += 4;
  }
  else
  {
    start_line = 0;
    lines_to_change = height;
  }

  row_ptr = row_inc * start_line;
  while (lines_to_change--)
  {
    pixel_ptr = row_ptr + ((encoded[stream_ptr++] - 1) * bytes_per_pixel);

    while (stream_ptr < encoded_size &&
           (rle_code = (signed char)encoded[stream_ptr++]) != -1)
    {
      if (rle_code == 0)
        // there's another skip code in the stream
        pixel_ptr += ((encoded[stream_ptr++] - 1) * bytes_per_pixel);
      else if (rle_code < 0)
      {
        // decode the run length code
        rle_code = -rle_code;
        stream_ptr++; // Ignore alpha channel
        r = encoded[stream_ptr++];
        g = encoded[stream_ptr++];
        b = encoded[stream_ptr++];
        while (rle_code--)
        {
          decoded[pixel_ptr++] = b;
          decoded[pixel_ptr++] = g;
          decoded[pixel_ptr++] = r;
          if (bytes_per_pixel == 4)
            pixel_ptr++;
        }
      }
      else
      {
        // copy pixels directly to output
        while (rle_code--)
        {
          stream_ptr++; // Ignore alpha channel
          decoded[pixel_ptr++] = encoded[stream_ptr + 2];
          decoded[pixel_ptr++] = encoded[stream_ptr + 1];
          decoded[pixel_ptr++] = encoded[stream_ptr + 0];
          stream_ptr += 3;
          if (bytes_per_pixel == 4)
            pixel_ptr++;
        }
      }
    }

    row_ptr += row_inc;
  }
}


typedef struct
  {
  void (*decode)(unsigned char *encoded,
                 int encoded_size,
                 unsigned char *decoded,
                 int width,
                 int height,
                 int bytes_per_pixel);
  gavl_video_frame_t * frame;
  uint8_t * buffer;
  int bytes_per_pixel;
  } qtrle_priv_t;

int init_qtrle(bgav_stream_t * s)
  {
  qtrle_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  switch(s->data.video.depth)
    {
    case 16:
      priv->decode = qt_decode_rle16;
      priv->buffer = malloc(s->data.video.format.image_width *
                            s->data.video.format.image_height * 2);

      priv->frame = gavl_video_frame_create(NULL);
      priv->frame->planes[0] = priv->buffer;
      priv->frame->strides[0] = s->data.video.format.image_width * 2;
      s->data.video.format.colorspace = GAVL_RGB_15;
      priv->bytes_per_pixel = 2;
      break;
    case 24:
      priv->decode = qt_decode_rle24;
      priv->buffer = malloc(s->data.video.format.image_width *
                            s->data.video.format.image_height * 3);

      priv->frame = gavl_video_frame_create(NULL);
      priv->frame->planes[0] = priv->buffer;
      priv->frame->strides[0] = s->data.video.format.image_width * 3;
      s->data.video.format.colorspace = GAVL_BGR_24;
      priv->bytes_per_pixel = 3;
      break;
    case 32:
      priv->decode = qt_decode_rle32;
      priv->buffer = malloc(s->data.video.format.image_width *
                            s->data.video.format.image_height * 3);

      priv->frame = gavl_video_frame_create(NULL);
      priv->frame->planes[0] = priv->buffer;
      priv->frame->strides[0] = s->data.video.format.image_width * 3;
      s->data.video.format.colorspace = GAVL_BGR_24;
      priv->bytes_per_pixel = 3;
      break;
    default:
      fprintf(stderr, "Unsupported depth %d\n", s->data.video.depth);
      return 0;
    }
  return 1;
  }

int decode_qtrle(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  qtrle_priv_t * priv;
  bgav_packet_t * p;
  priv = (qtrle_priv_t *)(s->data.video.decoder->priv);

  /* We assume that we get exactly one frame per packet */
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);

  if(!p)
    {
    fprintf(stderr, "Got no packet\n");
    return 0;
    }

  //  fprintf(stderr, "Got packet %d bytes\n", p->data_size);
  
  priv->decode(p->data,
               p->data_size,
               priv->buffer,
               s->data.video.format.image_width,
               s->data.video.format.image_height,
               priv->bytes_per_pixel);
  gavl_video_frame_copy(&(s->data.video.format), frame, priv->frame);
  
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

void close_qtrle(bgav_stream_t * s)
  {
  qtrle_priv_t * priv;
  priv = (qtrle_priv_t *)(s->data.video.decoder->priv);
  if(priv->buffer)
    free(priv->buffer);
  free(priv);
  }

static bgav_video_decoder_t decoder =
  {
    name:   "Quicktime rle decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('r', 'l', 'e', ' '),
                            0x00  },
    init:   init_qtrle,
    decode: decode_qtrle,
    close:  close_qtrle,
  };

void bgav_init_video_decoders_qtrle()
  {
  bgav_video_decoder_register(&decoder);
  }

#if 0

void qt_decode_rle(
  unsigned char *encoded,
  int encoded_size,
  unsigned char *decoded,
  int width,
  int height,
  int encoded_bpp,
  int bytes_per_pixel)
{
  switch (encoded_bpp)
  {
    case 8:
      qt_decode_rle8(
        encoded,
        encoded_size,
        decoded,
        width,
        height,
        bytes_per_pixel);
      break;
    case 16:
      qt_decode_rle16(
        encoded,
        encoded_size,
        decoded,
        width,
        height,
        bytes_per_pixel);
      break;
    case 24:
      qt_decode_rle24(
        encoded,
        encoded_size,
        decoded,
        width,
        height,
        bytes_per_pixel);
      break;
    case 32:
      qt_decode_rle32(
        encoded,
        encoded_size,
        decoded,
        width,
        height,
        bytes_per_pixel);
      break;
  }
}
#endif
