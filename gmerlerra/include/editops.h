/* Edit operations */

enum bg_nle_edit_op_e
  {
    BG_NLE_EDIT_ADD_TRACK,
    BG_NLE_EDIT_DELETE_TRACK,
    BG_NLE_EDIT_MOVE_TRACK,
    BG_NLE_EDIT_ADD_OUTSTREAM,
    BG_NLE_EDIT_DELETE_OUTSTREAM,
    BG_NLE_EDIT_MOVE_OUTSTREAM,
  };

// BG_NLE_EDIT_ADD_TRACK
// BG_NLE_EDIT_DELETE_TRACK
typedef struct
  {
  bg_nle_track_t * track;
  int index;
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

/* Undo data */

struct bg_nle_undo_data_s
  {
  bg_nle_edit_op_t op;
  void * data;
  bg_nle_undo_data_t * next;
  };

void bg_nle_undo_data_reverse(bg_nle_undo_data_t *);

void bg_nle_project_undo(bg_nle_project_t * project);
void bg_nle_project_redo(bg_nle_project_t * project);
