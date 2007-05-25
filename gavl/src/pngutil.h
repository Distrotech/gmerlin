
void write_png(char * filename, gavl_video_format_t * format,
               gavl_video_frame_t * frame);

gavl_video_frame_t * read_png(const char * filename,
                              gavl_video_format_t * format,
                              gavl_pixelformat_t pixelformat);
