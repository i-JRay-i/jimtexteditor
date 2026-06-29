#include "buffer.h"
#include "fileio.h"

/* Helper ERow functionalities */
/* Copies the content of an ERow struct to another one */
static void bufferCopyERow (const ERow *src_row, ERow *dst_row, int col_start, int col_end) {
  if (!src_row || !dst_row)
    return;

  if (col_start < 0) col_start = 0;
  if (col_end < col_start) col_end = col_start;
  if (col_end > src_row->row_len) col_end = src_row->row_len;

  int copy_len = col_end - col_start;
  dst_row->row_len = copy_len;
  dst_row->row_str = malloc(sizeof(char) * (copy_len + 1));
  if (copy_len > 0)
    memcpy(dst_row->row_str, src_row->row_str + col_start, copy_len);
  dst_row->row_str[copy_len] = '\0';

  dst_row->rndr_str = NULL;
  dst_row->rndr_len = 0;
  dst_row->rndr_cls = NULL;
}

/* Frees an ERow */
static void bufferFreeERow (void) {
  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
    memset(E.erow[row_idx].row_str, 0, E.erow[row_idx].row_len);
    memset(E.erow[row_idx].rndr_str, 0, E.erow[row_idx].rndr_len);
    //memset(E.erow[row_idx].rndr_cls, 0, E.erow[row_idx].rndr_len);
    free(E.erow[row_idx].row_str);
    free(E.erow[row_idx].rndr_str);
    free(E.erow[row_idx].rndr_cls);
    E.erow[row_idx].row_len = 0;
    E.erow[row_idx].rndr_len = 0;
  }
  E.num_row = 0;
}

/* Frees up the editor clipboard */
void bufferFreeEClip (void) {
  if (!E.eclip || !E.eclip->eclip_buff)
    return;

  for (int row_idx = 0; row_idx < E.eclip->num_row_eclip; row_idx++) {
    free(E.eclip->eclip_buff[row_idx].row_str);
    free(E.eclip->eclip_buff[row_idx].rndr_str);
    E.eclip->eclip_buff[row_idx].row_len = 0;
    E.eclip->eclip_buff[row_idx].rndr_len = 0;
  }
  free(E.eclip->eclip_buff);
  E.eclip->eclip_buff = NULL;
  E.eclip->num_row_eclip = 0;
  E.eclip->clip_type = CLIPBOARD_NONE;
}

/* Copies the content indicated by the start and end positions into
 * the clipboard. 
 */
void bufferCopyToEClip (int row_start, int row_end, int col_start, int col_end) {
  if (!E.eclip || E.num_row == 0)
    return;
  if (row_start < 0 || row_end < 0 || row_start > row_end || row_end >= E.num_row)
    return;

  bufferFreeEClip();

  int is_inline = (row_start == row_end &&
                   (col_start != 0 || col_end != E.erow[row_start].row_len));
  int num_rows = is_inline ? 1 : (row_end - row_start + 1);

  E.eclip->num_row_eclip = num_rows;
  E.eclip->clip_type = is_inline ? CLIPBOARD_INLINE : CLIPBOARD_LINE;
  E.eclip->eclip_buff = calloc(num_rows, sizeof(ERow));

  if (is_inline) {
    bufferCopyERow(&E.erow[row_start], &E.eclip->eclip_buff[0], col_start, col_end);
    return;
  }

  for (int row_idx = 0; row_idx < num_rows; row_idx++) {
    int x_start = (row_idx == 0) ? col_start : 0;
    int x_end = (row_idx == num_rows - 1) ? col_end : E.erow[row_start + row_idx].row_len;
    bufferCopyERow(&E.erow[row_start + row_idx], &E.eclip->eclip_buff[row_idx], x_start, x_end);
  }
}

/* Pastes the content of the clipboard on the cursor position */
void bufferPasteEClip (void) {
  if (!E.eclip || E.eclip->num_row_eclip == 0)
    return;

  if (E.eclip->clip_type == CLIPBOARD_INLINE) {
    if (E.num_row == 0)
      erowAppend(0, "", 0);
    if (E.crsr_y < 0)
      E.crsr_y = 0;
    if (E.crsr_y >= E.num_row)
      E.crsr_y = E.num_row - 1;

    ERow *dst = &E.erow[E.crsr_y];
    if (E.crsr_x < 0)
      E.crsr_x = 0;
    if (E.crsr_x > dst->row_len)
      E.crsr_x = dst->row_len;
    erowInsertString(dst, E.crsr_x, E.eclip->eclip_buff[0].row_str,
                     E.eclip->eclip_buff[0].row_len);
    E.crsr_x += E.eclip->eclip_buff[0].row_len;
    return;
  }

  int insert_row = E.crsr_y;
  for (int row_idx = 0; row_idx < E.eclip->num_row_eclip; row_idx++) {
    ERow *src_row = &E.eclip->eclip_buff[row_idx];
    erowAppend(insert_row + row_idx, src_row->row_str, src_row->row_len);
  }
}

/* Frees up all editor buffer elements */
void bufferFreeBuffer (int buffer_idx) {
  for (int row_idx = 0; row_idx < E.ebuff->num_row_hbuff[buffer_idx]; row_idx++) {
    memset(E.ebuff->erow_hbuff[buffer_idx][row_idx].row_str, 0, E.ebuff->erow_hbuff[buffer_idx][row_idx].row_len);
    memset(E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_str, 0, E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len);
    //memset(E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_cls, 0, E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len);
    free(E.ebuff->erow_hbuff[buffer_idx][row_idx].row_str);
    free(E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_str);
    //free(E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_cls);
    E.ebuff->erow_hbuff[buffer_idx][row_idx].row_len = 0;
    E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len = 0;
  }
  free(E.ebuff->erow_hbuff[buffer_idx]);
  E.ebuff->num_row_hbuff[buffer_idx] = 0;
  E.ebuff->crsr_x_hbuff[buffer_idx] = 0;
  E.ebuff->crsr_y_hbuff[buffer_idx] = 0;
}

/* Copies the current ERow array of the editor into the buffer at the buffer index. */
void bufferCopyERowToBuffer (int buffer_idx) {
  E.ebuff->crsr_x_hbuff[buffer_idx] = E.crsr_x;
  E.ebuff->crsr_y_hbuff[buffer_idx] = E.crsr_y;
  E.ebuff->num_row_hbuff[buffer_idx] = E.num_row;
  E.ebuff->erow_hbuff[buffer_idx] = malloc(sizeof(ERow) * E.ebuff->num_row_hbuff[buffer_idx]);

  for (int row_idx = 0; row_idx < E.ebuff->num_row_hbuff[buffer_idx]; row_idx++) {
    E.ebuff->erow_hbuff[buffer_idx][row_idx].row_len = E.erow[row_idx].row_len;
    E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len = E.erow[row_idx].rndr_len;

    // Generating each ERow in the editor
    E.ebuff->erow_hbuff[buffer_idx][row_idx].row_str = malloc(sizeof(char) * (E.ebuff->erow_hbuff[buffer_idx][row_idx].row_len+1));
    E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_str = malloc(sizeof(char) * (E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len+1));
    E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_cls = NULL; //malloc(sizeof(char) * (E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len+1)); // Still not implemented

    // Copying ERow strings
    strcpy(E.ebuff->erow_hbuff[buffer_idx][row_idx].row_str, E.erow[row_idx].row_str);
    strcpy(E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_str, E.erow[row_idx].rndr_str);
    //strcpy(E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_cls, E.erow[row_idx].rndr_cls);
  }
}

/* Copies the buffer at buffer index to the editor's ERow array. */
void bufferCopyBufferToERow (int buffer_idx) {
  E.crsr_x = E.ebuff->crsr_x_hbuff[buffer_idx];
  E.crsr_y = E.ebuff->crsr_y_hbuff[buffer_idx];
  E.num_row = E.ebuff->num_row_hbuff[buffer_idx];
  E.erow = malloc(sizeof(ERow) * E.num_row);

  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
    E.erow[row_idx].row_len = E.ebuff->erow_hbuff[buffer_idx][row_idx].row_len;
    E.erow[row_idx].rndr_len = E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_len;

    E.erow[row_idx].row_str = malloc(sizeof(char) * (E.erow[row_idx].row_len+1));
    E.erow[row_idx].rndr_str = malloc(sizeof(char) * (E.erow[row_idx].rndr_len+1));
    E.erow[row_idx].rndr_cls = malloc(sizeof(char) * (E.erow[row_idx].rndr_len+1));

    strcpy(E.erow[row_idx].row_str, E.ebuff->erow_hbuff[buffer_idx][row_idx].row_str);
    strcpy(E.erow[row_idx].rndr_str, E.ebuff->erow_hbuff[buffer_idx][row_idx].rndr_str);
    strcpy(E.erow[row_idx].rndr_cls, "");
  }
}

/* The current editor state is saved.
 * If the editor buffer is full, the index is shifted by one, and the last buffer index is filled.
 */
void bufferSaveEditorState (void) {
  if (E.ebuff->hbuff_idx < HIST_BUFF_STACK_SIZE) {
    if (E.ebuff->erow_hbuff[E.ebuff->hbuff_idx])
      bufferFreeBuffer(E.ebuff->hbuff_idx);
    bufferCopyERowToBuffer(E.ebuff->hbuff_idx);
    E.ebuff->hbuff_idx++;
    E.ebuff->hbuff_len = E.ebuff->hbuff_idx;
  } else {
    bufferFreeBuffer(0);
    for (int idx = 0; idx < HIST_BUFF_STACK_SIZE-1; idx++) {    // Shifting the buffer index below one
      E.ebuff->crsr_x_hbuff[idx] = E.ebuff->crsr_x_hbuff[idx+1];
      E.ebuff->crsr_y_hbuff[idx] = E.ebuff->crsr_y_hbuff[idx+1];
      E.ebuff->num_row_hbuff[idx] = E.ebuff->num_row_hbuff[idx+1];
      E.ebuff->erow_hbuff[idx] = E.ebuff->erow_hbuff[idx+1];
    }
    bufferCopyERowToBuffer(HIST_BUFF_STACK_SIZE-1);
  }
}

void bufferEditorUndo (void) {
  if (E.ebuff->hbuff_idx > 1) {
    if (E.erow)
      bufferFreeERow();
    E.ebuff->hbuff_idx--;
    bufferCopyBufferToERow(E.ebuff->hbuff_idx-1);
  } else {
    return;
  }
}

void bufferEditorRedo (void) {
  if (E.ebuff->hbuff_idx < E.ebuff->hbuff_len) {
    if (E.erow)
      bufferFreeERow();
    E.ebuff->hbuff_idx++;
    bufferCopyBufferToERow(E.ebuff->hbuff_idx-1);
  } else {
    return;
  }
}
