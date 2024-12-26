#include "input.h"

void insertChar(int ch) {
  if (E.crsr_y == E.num_row)
    erowAppend(E.num_row, "",0);
  erowInsertChar(&E.erow[E.crsr_y], E.crsr_x, ch);
  E.crsr_x++;
}

void deleteChar(void) {
  if (E.crsr_y == E.num_row)
    return;
  if (E.crsr_x == 0 && E.crsr_y == 0)
    return;

  ERow *erow = &E.erow[E.crsr_y];
  if (E.crsr_x > 0) {
    erowDeleteChar(erow, E.crsr_x - 1);
    E.crsr_x--;
  } else {
    E.crsr_x = E.erow[E.crsr_y-1].row_len;
    erowAppendString(&E.erow[E.crsr_y-1], erow->row_str, erow->row_len);
    erowDelete(E.crsr_y);
    E.crsr_y--;
  }
}
