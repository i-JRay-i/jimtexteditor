#include "input.h"

void insertChar(int ch) {
  if (E.crsr_y == E.num_row)
    erowAppend("",0);
  erowInsertChar(&E.erow[E.crsr_y], E.crsr_x, ch);
  E.crsr_x++;
}
