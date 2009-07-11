/* Edit operations */

enum bg_nle_edit_op_e
  {
    BG_NLE_EDIT_ADD_TRACK,
    BG_NLE_EDIT_DELETE_TRACK,
    BG_NLE_EDIT_MOVE_TRACK,
    BG_NLE_EDIT_ADD_OUTSTREAM,
    BG_NLE_EDIT_DELETE_OUTSTREAM,
    BG_NLE_EDIT_MOVE_OUTSTREAM,
    BG_NLE_EDIT_CHANGE_SELECTION,
    BG_NLE_EDIT_CHANGE_VISIBLE,
    BG_NLE_EDIT_CHANGE_ZOOM,
    BG_NLE_EDIT_TRACK_FLAGS,
    BG_NLE_EDIT_OUTSTREAM_FLAGS,
    BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK,
    BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK,
  };

// BG_NLE_EDIT_ADD_TRACK
// BG_NLE_EDIT_DELETE_TRACK
typedef struct
  {
  bg_nle_track_t * track;
  int index;

  int num_outstreams;
  bg_nle_outstream_t ** outstreams;
  
  } bg_nle_op_track_t;

// BG_NLE_EDIT_MOVE_TRACK
typedef struct
  {
  int old_index;
  int new_index;
  } bg_nle_op_move_track_t;

// BG_NLE_EDIT_ADD_OUTSTREAM
// BG_NLE_EDIT_DELETE_OUTSTREAM
typedef struct
  {
  bg_nle_outstream_t * outstream;
  int index;
  } bg_nle_op_outstream_t;

// BG_NLE_EDIT_MOVE_OUTSTREAM
typedef struct
  {
  int old_index;
  int new_index;
  } bg_nle_op_move_outstream_t;

// BG_NLE_EDIT_CHANGE_SELECTION
// BG_NLE_EDIT_CHANGE_VISIBLE
// BG_NLE_EDIT_CHANGE_ZOOM

typedef struct
  {
  bg_nle_time_range_t old_range;
  bg_nle_time_range_t new_range;
  } bg_nle_op_change_range_t;

// BG_NLE_EDIT_TRACK_FLAGS,

typedef struct
  {
  bg_nle_track_t * track;
  int old_flags;
  int new_flags;
  } bg_nle_op_track_flags_t;

// BG_NLE_EDIT_OUTSTREAM_FLAGS,

typedef struct
  {
  bg_nle_outstream_t * outstream;
  int old_flags;
  int new_flags;
  } bg_nle_op_outstream_flags_t;

// BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK
// BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK

typedef struct
  {
  bg_nle_outstream_t * outstream;
  bg_nle_track_t     * track;
  } bg_nle_op_outstream_track_t;

/* Undo data */

struct bg_nle_undo_data_s
  {
  bg_nle_edit_op_t op;
  void * data;
  bg_nle_undo_data_t * next;
  };

void bg_nle_project_push_undo(bg_nle_project_t * p, bg_nle_undo_data_t * data);
void bg_nle_undo_data_destroy(bg_nle_undo_data_t * data);

void bg_nle_undo_data_reverse(bg_nle_undo_data_t *);

void bg_nle_project_undo(bg_nle_project_t * project);
void bg_nle_project_redo(bg_nle_project_t * project);
