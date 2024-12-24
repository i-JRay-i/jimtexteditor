#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "fileio.h"

void erowRender(ERow *erow) {
  int num_tabs = 0;
  for (int row_idx = 0; row_idx < erow->row_len; row_idx++) {
    if (erow->row_str[row_idx] == '\t')
      num_tabs++;
  }

  free(erow->rndr_str);
  erow->rndr_str = malloc(erow->row_len + num_tabs*(TAB_SIZE - 1) + 1);

  int rndr_idx = 0;
  for (int row_idx=0; row_idx<erow->row_len; row_idx++) {
    if (erow->row_str[row_idx] == '\t') {
      erow->rndr_str[rndr_idx++] =  ' ';
      while (rndr_idx % TAB_SIZE != 0)
        erow->rndr_str[rndr_idx++] =  ' ';
    } else {
      erow->rndr_str[rndr_idx++] = erow->row_str[row_idx];
    }
  }

  erow->rndr_str[rndr_idx] = '\0';
  erow->rndr_len = rndr_idx;
}

void erowAppend(char *str, size_t len) {
  E.erow = realloc(E.erow, sizeof(ERow)*(E.num_row+1));

  int cur_row = E.num_row;
  E.erow[cur_row].row_len = len;
  E.erow[cur_row].row_str = malloc(len+1);
  memcpy(E.erow[cur_row].row_str, str, len);
  E.erow[cur_row].row_str[len] = '\0';

  E.erow[cur_row].rndr_len = 0;
  E.erow[cur_row].rndr_str = NULL;
  erowRender(&E.erow[cur_row]);

  E.num_row += 1;
}

void erowInsertChar(ERow *erow, int curr, int ch) {
  if (curr < 0 || curr > erow->row_len)
    curr = erow->row_len;
  erow->row_str = realloc(erow->row_str, erow->row_len + 2);
  memmove(&erow->row_str[curr+1], &erow->row_str[curr], erow->row_len-curr+1);
  erow->row_len++;
  erow->row_str[curr] = ch;
  erowRender(erow);
}

char *writeFile(int *len_file) {
  int len_total = 0;
  for (int row_idx = 0; row_idx < E.num_row; row_idx++)
    len_total += E.erow[row_idx].row_len + 1;
  *len_file = len_total;

  char *file = malloc(len_total);
  char *p_file = file;
  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
    memcpy(p_file, E.erow[row_idx].row_str, E.erow[row_idx].row_len);
    p_file += E.erow[row_idx].row_len;
    *p_file = '\n';
    p_file++;
  }
  return file;
}

void saveFile(void) {
  if (E.filename == NULL) return;

  int len = 0;
  char *file = writeFile(&len);

  int fd = open(E.filename, (O_RDWR | O_CREAT), 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, file, len) == len) {
        close(fd);
        free(file);
        return;
      }
    }
    close(fd);
  }

  free(file);
}

void openFile(char *filename) {
  E.filename = filename;

  FILE *p_file = fopen(filename, "r");
  if (!p_file)
    die("fopen");

  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len = 0;

  while ((line_len = getline(&line, &line_cap, p_file)) != -1) {
    while (line_len > 0 && (line[line_len-1] == '\n' || line[line_len-1] == '\r')) {
      line_len -= 1;
    }
    erowAppend(line, line_len);
  }

  free(line);
  fclose(p_file);
}
