#include "buffer.h"

void bufferFreeBuffer (int buffer_idx) {
  for (int row_idx = 0; row_idx < E.ebuff->num_row_buff[buffer_idx]; row_idx++) {
    free(E.ebuff->erow_buff[buffer_idx][row_idx].row_str);
    free(E.ebuff->erow_buff[buffer_idx][row_idx].rndr_str);
    //free(E.ebuff->erow_buff[buffer_idx][idx].rndr_cls);
    E.ebuff->erow_buff[buffer_idx][row_idx].row_len = 0;
    E.ebuff->erow_buff[buffer_idx][row_idx].rndr_len = 0;
  }
  free(E.ebuff->erow_buff[buffer_idx]);
  E.ebuff->num_row_buff[buffer_idx] = 0;
  E.ebuff->crsr_x_buff[buffer_idx] = 0;
  E.ebuff->crsr_y_buff[buffer_idx] = 0;
}

void bufferFreeERow (void) {
  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
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
  E.ebuff->crsr_x_buff[buffer_idx] = E.crsr_x;
  E.ebuff->crsr_y_buff[buffer_idx] = E.crsr_y;
  E.ebuff->num_row_buff[buffer_idx] = E.num_row;
  E.ebuff->erow_buff[buffer_idx] = malloc(sizeof(ERow) * E.ebuff->num_row_buff[buffer_idx]);

  for (int row_idx = 0; row_idx < E.ebuff->num_row_buff[buffer_idx]; row_idx++) {
    E.ebuff->erow_buff[buffer_idx][row_idx].row_len = E.erow[row_idx].row_len;
    E.ebuff->erow_buff[buffer_idx][row_idx].rndr_len = E.erow[row_idx].rndr_len;

    // Generating each ERow in the editor
    E.ebuff->erow_buff[buffer_idx][row_idx].row_str = malloc(sizeof(char) * (E.ebuff->erow_buff[buffer_idx][row_idx].row_len+1));
    E.ebuff->erow_buff[buffer_idx][row_idx].rndr_str = malloc(sizeof(char) * (E.ebuff->erow_buff[buffer_idx][row_idx].rndr_len+1));
    E.ebuff->erow_buff[buffer_idx][row_idx].rndr_cls = NULL; // Still not implemented
    //E.ebuff->erow_buff[buffer_idx][row_idx].rndr_cls = malloc(sizeof(char) * (E.ebuff->erow_buff[buffer_idx][row_idx].rndr_len+1));

    // Copying ERow strings
    strcpy(E.ebuff->erow_buff[buffer_idx][row_idx].row_str, E.erow[row_idx].row_str);
    strcpy(E.ebuff->erow_buff[buffer_idx][row_idx].rndr_str, E.erow[row_idx].rndr_str);
    //strcpy(E.ebuff->erow_buff[buffer_idx][row_idx].rndr_cls, E.erow[row_idx].rndr_cls);
  }
}

/* Copies the buffer at buffer index to the editor's ERow array. */
void bufferCopyBufferToERow (int buffer_idx) {
  E.crsr_x = E.ebuff->crsr_x_buff[buffer_idx];
  E.crsr_y = E.ebuff->crsr_y_buff[buffer_idx];
  E.num_row = E.ebuff->num_row_buff[buffer_idx];
  E.erow = malloc(sizeof(ERow) * E.num_row);

  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
    E.erow[row_idx].row_len = E.ebuff->erow_buff[buffer_idx][row_idx].row_len;
    E.erow[row_idx].rndr_len = E.ebuff->erow_buff[buffer_idx][row_idx].rndr_len;

    E.erow[row_idx].row_str = malloc(sizeof(char) * (E.erow[row_idx].row_len + 1));
    E.erow[row_idx].rndr_str = malloc(sizeof(char) * (E.erow[row_idx].rndr_len + 1));
    E.erow[row_idx].rndr_cls = malloc(sizeof(char) * (E.erow[row_idx].rndr_len + 1));

    strcpy(E.erow[row_idx].row_str, E.ebuff->erow_buff[buffer_idx][row_idx].row_str);
    strcpy(E.erow[row_idx].rndr_str, E.ebuff->erow_buff[buffer_idx][row_idx].rndr_str);
    strcpy(E.erow[row_idx].rndr_cls, "");
  }
}

/* The current editor state is saved at each exit from the insert mode. 
 * If the editor buffer is full, the index is shifted by one, and the last buffer index is filled.
 */
void bufferSaveEditorState (void) {
  if (E.ebuff->buff_idx < HIST_BUFF_STACK_SIZE) {
    if (E.ebuff->erow_buff[E.ebuff->buff_idx])
      bufferFreeBuffer(E.ebuff->buff_idx);
    bufferCopyERowToBuffer(E.ebuff->buff_idx);
    E.ebuff->buff_idx++;
    E.ebuff->buff_len = E.ebuff->buff_idx;
  } else {
    bufferFreeBuffer(0);
    for (int idx = 0; idx < HIST_BUFF_STACK_SIZE-1; idx++) {    // Shifting the buffer index below one
      E.ebuff->crsr_x_buff[idx] = E.ebuff->crsr_x_buff[idx+1];
      E.ebuff->crsr_y_buff[idx] = E.ebuff->crsr_y_buff[idx+1];
      E.ebuff->num_row_buff[idx] = E.ebuff->num_row_buff[idx+1];
      E.ebuff->erow_buff[idx] = E.ebuff->erow_buff[idx+1];
    }
    bufferCopyERowToBuffer(HIST_BUFF_STACK_SIZE-1);
  }
}

void bufferEditorUndo (void) {
  if (E.ebuff->buff_idx > 1) {
    if (E.erow)
      bufferFreeERow();
    E.ebuff->buff_idx--;
    bufferCopyBufferToERow(E.ebuff->buff_idx-1);
  } else {
    return;
  }
}

void bufferEditorRedo (void) {
  if (E.ebuff->buff_idx < E.ebuff->buff_len) {
    if (E.erow)
      bufferFreeERow();
    E.ebuff->buff_idx++;
    bufferCopyBufferToERow(E.ebuff->buff_idx-1);
  } else {
    return;
  }
}
