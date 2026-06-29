#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "fileio.h"

char * writeFile (int * len_file) {
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

char * newsavePrompt (void) {
  size_t max_len = 256;
  char *input = malloc(max_len);
  size_t input_len = 0;
  char *prompt = "Save as:";
  input[0] = '\0';

  E.cmd.cmd_str[0] = '\0';
  E.cmd.cmd_len = 0;

  while (1) {
    appendMessageString(prompt, 8);
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
    editorRefreshScreen();
  }
}

void saveFile (void) {
  if (E.filename == NULL) {
    E.filename = newsavePrompt();
  }

  int len = 0;
  char * file = writeFile(&len);

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

void openFile (char * filename) {
  E.filename = malloc(sizeof(char) * (strlen(filename) + 1));
  strcpy(E.filename, filename);

  FILE * p_file = fopen(filename, "r");
  if (!p_file) {
    int fd = open(E.filename, (O_RDWR | O_CREAT), 0644);
    close(fd);
    p_file = fopen(filename, "r");
  }

  char * line = NULL;
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
