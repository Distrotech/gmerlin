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
    BG_NLE_EDIT_CHANGE_IN_OUT,
    BG_NLE_EDIT_CHANGE_VISIBLE,
    BG_NLE_EDIT_CHANGE_ZOOM,
    BG_NLE_EDIT_TRACK_FLAGS,
    BG_NLE_EDIT_OUTSTREAM_FLAGS,
    BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK,
    BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK,
    BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT,
    BG_NLE_EDIT_PROJECT_PARAMETERS,
    BG_NLE_EDIT_TRACK_PARAMETERS,
    BG_NLE_EDIT_OUTSTREAM_PARAMETERS,
    BG_NLE_EDIT_ADD_FILE,
    BG_NLE_EDIT_DELETE_FILE,
    BG_NLE_EDIT_SET_CURSOR_POS,
    BG_NLE_EDIT_SET_EDIT_MODE,
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
// BG_NLE_EDIT_CHANGE_IN_OUT

typedef struct
  {
  bg_nle_time_range_t old_range;
  bg_nle_time_range_t new_range;
  
  int64_t old_cursor_pos;
  int64_t new_cursor_pos;
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

// BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT

typedef struct
  {
  bg_nle_outstream_t * old_outstream;
  bg_nle_outstream_t * new_outstream;
  bg_nle_track_type_t type;
  } bg_nle_op_outstream_make_current_t;

// BG_NLE_EDIT_PROJECT_PARAMETERS,
// BG_NLE_EDIT_TRACK_PARAMETERS,
// BG_NLE_EDIT_OUTSTREAM_PARAMETERS,

typedef struct
  {
  bg_cfg_section_t * old_section;
  bg_cfg_section_t * new_section;
  int index; // Track or outstream index
  } bg_nle_op_parameters_t;

// BG_NLE_EDIT_ADD_FILE,
// BG_NLE_EDIT_DELETE_FILE,

typedef struct
  {
  bg_nle_file_t * file;
  int index; // index
  } bg_nle_op_file_t;

// BG_NLE_EDIT_SET_CURSOR_POS,

typedef struct
  {
  int64_t old_pos;
  int64_t new_pos;
  } bg_nle_op_cursor_pos_t;

// BG_NLE_EDIT_SET_EDIT_MODE,

typedef struct
  {
  int old_mode;
  int new_mode;
  } bg_nle_op_edit_mode_t;

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
