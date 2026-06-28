#include "buffer.h"

void bufferFreeEClip (void) {
  return;
}

void bufferCopyToEClip (void) {
  return;
}

void bufferPasteEClip (void) {
  return;
}

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

void bufferFreeERow (void) {
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

/* The current editor state is saved at each exit from the insert mode. 
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
