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

void erowAppend(int cur_row, char *str, size_t len) {
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
  erowRender(&E.erow[cur_row]);

  E.num_row++;
  E.dirt_flag_pos++;
}

void erowAppendString(ERow *erow, char *str, size_t str_len) {
  erow->row_str = realloc(erow->row_str, erow->row_len+str_len+1);
  memcpy(&erow->row_str[erow->row_len], str, str_len);
  erow->row_len+=str_len;
  erow->row_str[erow->row_len] = '\0';
  erowRender(erow);
  E.dirt_flag_pos++;
}

void erowInsertChar(ERow *erow, int curr, int ch) {
  if (curr < 0 || curr > erow->row_len)
    curr = erow->row_len;
  erow->row_str = realloc(erow->row_str, erow->row_len + 2);
  memmove(&erow->row_str[curr+1], &erow->row_str[curr], erow->row_len-curr+1);
  erow->row_len++;
  erow->row_str[curr] = ch;

  erowRender(erow);
  E.dirt_flag_pos++;
}

void erowInsertRow(void) {
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

void erowDeleteChar(ERow *erow, int crsr_pos) {
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

void erowDelete(int curr_row) {
  if (curr_row < 0 || curr_row >= E.num_row)
    return;
  erowFree(&E.erow[curr_row]);
  memmove(&E.erow[curr_row], &E.erow[curr_row+1], sizeof(ERow) * (E.num_row-curr_row-1));
  E.num_row--;
  E.dirt_flag_neg++;
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

char *newsavePrompt(char *prompt, int prompt_len) {
  size_t max_len = 256;
  char *input = malloc(max_len);
  size_t input_len = 0;
  input[0] = '\0';

  E.cmd.cmd_str[0] = '\0';
  E.cmd.cmd_len = 0;

  while (1) {
    appendMessageString(prompt, prompt_len);
    int ch = readKey();
    if (ch == '\r') {
      if (input_len != 0) {
        appendMessageString("", 0);
        return input;
      }
    } else if (!iscntrl(ch) && ch < 128) {
      if (input_len == max_len-1) {
        max_len += 255;
        input = realloc(input, max_len);
      }
      input[input_len++] = ch;
      input[input_len] = '\0';
    }
    appendMessageString(input, input_len);
    refreshScreen();
  }
}

void saveFile(void) {
  if (E.filename == NULL) {
    E.filename = newsavePrompt("Save as", 7);
  }

  int len = 0;
  char *file = writeFile(&len);

  int fd = open(E.filename, (O_RDWR | O_CREAT), 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, file, len) == len) {
        close(fd);
        free(file);
        E.dirt_flag_pos = 0;
        E.dirt_flag_neg = 0;
        setMessage("%d bytes written to disk.", len);
        return;
      }
    }
    close(fd);
  }

  free(file);
  setMessage("Can't save: I/O error: %s", strerror(errno));
}

void openFile(char *filename) {
  E.filename = filename;

  FILE *p_file = fopen(filename, "r");
  if (!p_file) {
    int fd = open(E.filename, (O_RDWR | O_CREAT), 0644);
    close(fd);
    p_file = fopen(filename, "r");
  }

  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len = 0;

  while ((line_len = getline(&line, &line_cap, p_file)) != -1) {
    while (line_len > 0 && (line[line_len-1] == '\n' || line[line_len-1] == '\r')) {
      line_len -= 1;
    }
    erowAppend(E.num_row, line, line_len);
  }

  free(line);
  fclose(p_file);
  E.dirt_flag_pos = 0;
}
