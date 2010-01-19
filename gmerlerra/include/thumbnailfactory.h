

typedef struct bg_nle_thumbnail_factory_s bg_nle_thumbnail_factory_t;

bg_nle_thumbnail_factory_t *
bg_nle_thumbnail_factory_create(bg_nle_project_t * p);

void bg_nle_thumbnail_factory_destroy(bg_nle_thumbnail_factory_t *);

const bg_parameter_info_t * bg_nle_thumbnail_factory_get_parameters(void);

void bg_nle_thumbnail_factory_set_parameter(void * data, const char * name,
                                            const bg_parameter_value_t * val);

int
bg_nle_thumbnail_factory_request_audio(bg_nle_thumbnail_factory_t *,
                                       bg_nle_id_t id, int stream,
                                       gavl_time_t start_time,
                                       gavl_time_t end_time,
                                       gavl_video_format_t * format,
                                       gavl_video_frame_t * frame);

void nle_thumbnail_factory_wait_audio(bg_nle_thumbnail_factory_t *, int id);

int
bg_nle_thumbnail_factory_request_video(bg_nle_thumbnail_factory_t *,
                                       bg_nle_id_t id, int stream,
                                       gavl_time_t start_time,
                                       gavl_time_t end_time,
                                       gavl_video_format_t * format,
                                       gavl_video_frame_t * frame);

void nle_thumbnail_factory_wait_video(bg_nle_thumbnail_factory_t *, int id);
