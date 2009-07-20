#include <avdec_private.h>
#include <bgav_vdpau.h>
#include <stdlib.h>

#define LOG_DOMAIN "vdpau"

struct bgav_vdpau_context_s
  {
  Display * dpy;
  VdpDevice device;
  VdpGetProcAddress *get_proc_address;

  /* Get error string */
  VdpGetErrorString * get_error_string;
  
  /* Surface */
  VdpVideoSurfaceCreate  * video_surface_create;
  VdpVideoSurfaceDestroy * video_surface_destroy;
  VdpVideoSurfaceGetBitsYCbCr * video_surface_get_bits_ycbcr;
  
  /* Decoder */
  VdpDecoderCreate  * decoder_create;
  VdpDecoderDestroy * decoder_destroy;
  VdpDecoderRender * decoder_render;
  
  VdpDeviceDestroy * device_destroy;

  const bgav_options_t * opt;
  };

bgav_vdpau_context_t * bgav_vdpau_context_create(const bgav_options_t * opt)
  {
  bgav_vdpau_context_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->opt = opt;
  
  ret->dpy = XOpenDisplay(NULL);
  if(!ret->dpy)
    goto fail;

  ret->device = VDP_INVALID_HANDLE;
  
  /* Create device */
  if(vdp_device_create_x11(ret->dpy, DefaultScreen(ret->dpy), &ret->device,
                           &ret->get_proc_address) != VDP_STATUS_OK)
    goto fail;

  /* Get function pointers */

  /* Destroy device */
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_DEVICE_DESTROY,
                           (void**)&ret->device_destroy) != VDP_STATUS_OK)
    goto fail;
  
  /* get error string */
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_GET_ERROR_STRING,
                           (void**)&ret->get_error_string) != VDP_STATUS_OK)
    goto fail;
  
  /* Surface */
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_VIDEO_SURFACE_CREATE,
                           (void**)&ret->video_surface_create) != VDP_STATUS_OK)
    goto fail;
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_VIDEO_SURFACE_DESTROY,
                           (void**)&ret->video_surface_destroy) != VDP_STATUS_OK)
    goto fail;
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR,
                           (void**)&ret->video_surface_get_bits_ycbcr) != VDP_STATUS_OK)
    goto fail;

  /* Decoder */  
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_DECODER_CREATE,
                           (void**)&ret->decoder_create) != VDP_STATUS_OK)
    goto fail;
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_DECODER_DESTROY,
                           (void**)&ret->decoder_destroy) != VDP_STATUS_OK)
    goto fail;
  if(ret->get_proc_address(ret->device, VDP_FUNC_ID_DECODER_RENDER,
                           (void**)&ret->decoder_render) != VDP_STATUS_OK)
    goto fail;
  
  return ret;

  fail:
  bgav_vdpau_context_destroy(ret);
  return NULL;
  }

void bgav_vdpau_context_destroy(bgav_vdpau_context_t * ctx)
  {
  if(ctx->device != VDP_INVALID_HANDLE)
    ctx->device_destroy(ctx->device);
  
  free(ctx);
  }


VdpVideoSurface
bgav_vdpau_context_create_video_surface(bgav_vdpau_context_t * ctx,
                                        VdpChromaType chroma_type,
                                        uint32_t width, uint32_t height)
  {
  VdpVideoSurface ret;
  VdpStatus st;
  
  if((st = ctx->video_surface_create(ctx->device,
                                     chroma_type,
                                     width, height, &ret)) != VDP_STATUS_OK)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Creating surface failed: %s\n",
             ctx->get_error_string(st));
    return VDP_INVALID_HANDLE;
    }
  return ret;
  }

void bgav_vdpau_context_destroy_video_surface(bgav_vdpau_context_t * ctx,
                                              VdpVideoSurface s)
  {
  ctx->video_surface_destroy(s);
  }

VdpDecoder
bgav_vdpau_context_create_decoder(bgav_vdpau_context_t * ctx,
                                  VdpDecoderProfile profile,
                                  uint32_t width, uint32_t height,
                                  uint32_t max_references)
  {
  VdpDecoder ret;
  VdpStatus st;

  if((st = ctx->decoder_create(ctx->device, profile,
                               width, height,
                               max_references, &ret)) != VDP_STATUS_OK)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Creating decoder failed: %s\n",
             ctx->get_error_string(st));
    return VDP_INVALID_HANDLE;
    }
  return ret;
  }

void bgav_vdpau_context_destroy_decoder(bgav_vdpau_context_t * ctx,
                                        VdpDecoder s)
  {
  ctx->decoder_destroy(s);
  }

VdpStatus
bgav_vdpau_context_decoder_render(bgav_vdpau_context_t * ctx,
                                  VdpDecoder dec,
                                  VdpVideoSurface target,
                                  VdpPictureInfo const *picture_info,
                                  uint32_t bitstream_buffer_count,
                                  VdpBitstreamBuffer const *bitstream_buffers)
  {
  VdpStatus st;
  if((st = ctx->decoder_render(dec, target, picture_info, bitstream_buffer_count,
                               bitstream_buffers)) != VDP_STATUS_OK)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Decoding image failed: %s\n",
             ctx->get_error_string(st));
    }
  return st;
  }

void bgav_vdpau_context_surface_to_frame(bgav_vdpau_context_t * ctx,
                                         VdpVideoSurface s, gavl_video_frame_t * dst)
  {
  uint32_t pitches[4];
  const void * planes[4];
  VdpStatus st;
  pitches[0] = dst->strides[0];
  pitches[1] = dst->strides[2];
  pitches[2] = dst->strides[1];
  //  pitches[0] = 0;
  //  pitches[1] = 0;
  //  pitches[2] = 0;
  
  planes[0] = dst->planes[0];
  planes[1] = dst->planes[2];
  planes[2] = dst->planes[1];
#if 1
  if((st = ctx->video_surface_get_bits_ycbcr(s,
                                             // VDP_YCBCR_FORMAT_YUYV,
                                             VDP_YCBCR_FORMAT_YV12,
                                             (void *const *)planes, pitches)) !=
     VDP_STATUS_OK)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Get surface bits failed: %s\n",
             ctx->get_error_string(st));
    
    }
#endif
  }
