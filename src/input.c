#include "input.h"

/* Creates the render string of the ERow argument */
void erowRender (ERow * erow) {
  int num_tabs = 0;
  for (int row_idx = 0; row_idx < erow->row_len; row_idx++) {
    if (erow->row_str[row_idx] == '\t')
      num_tabs++;
  }

  free(erow->rndr_str);
  erow->rndr_str = malloc(erow->row_len + num_tabs*(TAB_SIZE-1) + 1);
  erow->rndr_cls = malloc(erow->row_len + num_tabs*(TAB_SIZE-1) + 1);

  int rndr_idx = 0;
  for (int row_idx=0; row_idx<erow->row_len; row_idx++) {
    if (erow->row_str[row_idx] == '\t') {
      erow->rndr_str[rndr_idx++] =  ' ';
      while (rndr_idx % TAB_SIZE != 0)
        erow->rndr_str[rndr_idx++] =  ' ';
    } else {
      erow->rndr_str[rndr_idx++] = erow->row_str[row_idx];
    }
    erow->rndr_cls = 0;
  }

  erow->rndr_str[rndr_idx] = '\0';
  erow->rndr_len = rndr_idx;
}

void erowAppend (int cur_row, char *str, size_t len) {
  if (cur_row < 0 || cur_row > E.num_row)
    return;
  E.erow = realloc(E.erow, sizeof(ERow)*(E.num_row+1));
  memmove(&E.erow[cur_row+1], &E.erow[cur_row], sizeof(ERow)*(E.num_row-cur_row));

  E.erow[cur_row].row_len = len;
  E.erow[cur_row].row_str = malloc(len+1);
  memcpy(E.erow[cur_row].row_str, str, len);
  E.erow[cur_row].row_str[len] = '\0';

  E.erow[cur_row].rndr_len = 0;
  E.erow[cur_row].rndr_str = NULL;
  E.erow[cur_row].rndr_cls = NULL;
  erowRender(&E.erow[cur_row]);

  E.num_row++;
  E.dirt_flag_pos++;
}

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
    erowDeleteRow(E.crsr_y);
    E.crsr_y--;
  }
}

void erowAppendString (ERow * erow, char *str, size_t str_len) {
  erow->row_str = realloc(erow->row_str, erow->row_len+str_len+1);
  memcpy(&erow->row_str[erow->row_len], str, str_len);
  erow->row_len+=str_len;
  erow->row_str[erow->row_len] = '\0';
  erowRender(erow);
  E.dirt_flag_pos++;
}

void erowInsertChar (ERow * erow, int curr, int ch) {
  if (curr < 0 || curr > erow->row_len)
    curr = erow->row_len;
  erow->row_str = realloc(erow->row_str, erow->row_len + 2);
  memmove(&erow->row_str[curr+1], &erow->row_str[curr], erow->row_len-curr+1);
  erow->row_len++;
  erow->row_str[curr] = ch;

  erowRender(erow);
  E.dirt_flag_pos++;
}

void erowInsertString(ERow *erow, int pos, char *s, size_t len) {
  if (pos < 0)
    pos = 0;
  if (pos > erow->row_len)
    pos = erow->row_len;

  erow->row_str = realloc(erow->row_str, erow->row_len + len + 1);
  memmove(&erow->row_str[pos + len], &erow->row_str[pos], erow->row_len - pos + 1);
  memcpy(&erow->row_str[pos], s, len);
  erow->row_len += len;
  erow->row_str[erow->row_len] = '\0';

  erowRender(erow);
  E.dirt_flag_pos++;
}

void erowInsertRow (void) {
  if (E.crsr_x == 0) {
    erowAppend(E.crsr_y, "", 0);
  } else {
    ERow *erow = &E.erow[E.crsr_y];
    erowAppend(E.crsr_y+1, &erow->row_str[E.crsr_x], erow->row_len - E.crsr_x);
    erow = &E.erow[E.crsr_y];
    erow->row_len = E.crsr_x;
    erow->row_str[erow->row_len] = '\0';
    erowRender(erow);
  }
  E.crsr_y++;
  E.crsr_x=0;
}

void erowDeleteChar (ERow *erow, int crsr_pos) {
  if (crsr_pos < 0 || crsr_pos >= erow->row_len)
    return;
  memmove(&erow->row_str[crsr_pos], &erow->row_str[crsr_pos+1], erow->row_len - crsr_pos);
  erow->row_len--;
  erowRender(erow);
  E.dirt_flag_neg++;
}

void erowFree(ERow *erow) {
  free(erow->rndr_str);
  free(erow->row_str);
}

void erowDeleteRow (int curr_row) {
  if (curr_row < 0 || curr_row >= E.num_row)
    return;
  erowFree(&E.erow[curr_row]);
  memmove(&E.erow[curr_row], &E.erow[curr_row+1], sizeof(ERow) * (E.num_row-curr_row-1));
  E.num_row--;
  E.dirt_flag_neg++;
}