#ifndef BG_NLE_TYPES_H
#define BG_NLE_TYPES_H

typedef int32_t bg_nle_id_t;

typedef enum
  {
    BG_NLE_TRACK_NONE = 0,
    BG_NLE_TRACK_AUDIO,
    BG_NLE_TRACK_VIDEO,
  } bg_nle_track_type_t;

/* WARNING: Changing these alters the file format */
#define BG_NLE_TRACK_SELECTED (1<<0)
#define BG_NLE_TRACK_EXPANDED (1<<1)
#define BG_NLE_TRACK_PLAYBACK (1<<2)

typedef struct bg_nle_project_s bg_nle_project_t;

typedef enum bg_nle_edit_op_e bg_nle_edit_op_t;

typedef struct bg_nle_undo_data_s bg_nle_undo_data_t;

typedef struct
  {
  int64_t start;
  int64_t end;
  } bg_nle_time_range_t;

void bg_nle_time_range_copy(bg_nle_time_range_t * dst, const bg_nle_time_range_t * src);
void bg_nle_time_range_swap(bg_nle_time_range_t * r1, bg_nle_time_range_t * r2);


#endif
