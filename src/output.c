#include "output.h"

OBuf ob;

void bufInit(OBuf *ob) {
  ob->p_str = 0;
  ob->size = 128;

  char *new_str = malloc(ob->size);
  ob->str = new_str;
}

void bufAppend(OBuf *ob, const char *str, int len) {
  if (ob->p_str+len > ob->size) {
    ob->size += 128;
    ob->str = realloc(ob->str, ob->size);
  }

  if (ob->str == NULL)
    return;

  memcpy(&ob->str[ob->p_str], str, len);
  ob->p_str += len;
}

void bufFree(OBuf *ob) {
  free(ob->str);
  ob->p_str = 0;
  ob->size = 0;
}

void editorScroll() {
  if (E.crsr_y < E.row_off)
    E.row_off = E.crsr_y;
  if (E.crsr_y >= E.row_off + E.term_height-2)
    E.row_off = E.crsr_y - E.term_height+3;
  if (E.crsr_x < E.col_off)
    E.col_off = E.crsr_x;
  if (E.crsr_x >= E.col_off + E.term_width)
    E.col_off = E.crsr_x - E.term_width+1;
}

/* Draws the screen */
void drawEditorScreen(OBuf *ob) {
  for (int row = 0; row < E.term_height-2; row++) {
    int curr_row = row + E.row_off;
    if (curr_row >= E.num_row) {
      if (E.num_row == 0 && row == E.term_height / 3) { // If there is no file opened
        char welcome_msg[80];
        int welcome_msg_len = snprintf(welcome_msg, sizeof(welcome_msg),
                                       "JIM Editor - Version %s", JIM_VERSION);
        if (welcome_msg_len > E.term_height)
          welcome_msg_len = E.term_height;
        int padding = (E.term_width - welcome_msg_len)/2;
        if (padding) {
          bufAppend(ob, "~", 1);
          padding--;
        }
        while (padding--)
          bufAppend(ob, " ", 1);
        bufAppend(ob, welcome_msg, welcome_msg_len);
      } else {
        bufAppend(ob, "~", 1);
      }
    } else { // If there is an ERow to print
      int row_len = E.erow[curr_row].rndr_len - E.col_off;
      if (row_len < 0)
        row_len = 0;
      if (row_len > E.term_width)
        row_len = E.term_width;
      bufAppend(ob, &E.erow[curr_row].rndr_str[E.col_off], row_len);
    }
    bufAppend(ob, "\x1b[K", 3);
    if (row < E.term_height - 2)
      bufAppend(ob, "\r\n", 2);
  }
}

void drawStatusBar(OBuf *ob) {        // Drawing the status bar
  int stat_str_crsr = 0;
  char stat_str_right[100];
  int right_len = snprintf(stat_str_right, 100, "%d/%d", E.crsr_y+1, E.num_row);

  bufAppend(ob, "\x1b[7m", 4);
  for (int stat_crsr = 0; stat_crsr < E.term_width - right_len - 1; stat_crsr++) {
    if (stat_crsr > 1 && stat_crsr < E.estat.stat_len + 2) {
      bufAppend(ob, &E.estat.stat_str[stat_str_crsr], 1);
      stat_str_crsr++;
    } else {
      bufAppend(ob, " ", 1);
    }
  }
  bufAppend(ob, stat_str_right, right_len);
  bufAppend(ob, " \x1b[m", 4);
  bufAppend(ob, "\r\n", 2);
}

void drawMessageBar(OBuf *ob) {
  int msg_str_crsr = 0;

  for (int msg_crsr = 0; msg_crsr < E.term_width; msg_crsr++) {
    if (msg_crsr > 1 && msg_crsr < E.emsg.msg_len + 2) {
      bufAppend(ob, &E.emsg.msg_str[msg_str_crsr], 1);
      msg_str_crsr++;
    } else {
      bufAppend(ob, " ", 1);
    }
  }
}

void setStatusString(void) {
  char stat_str[100];
  int stat_len = snprintf(stat_str, 100, "%.20s - %d lines, ",
                     E.filename ? E.filename : "[No Name", E.num_row);
  appendStatusString(stat_str, stat_len);

  if (E.emode == MODE_NORMAL) {
    appendStatusString("NORMAL", 6);
  } else if (E.emode == MODE_INSERT) {
    appendStatusString("INSERT", 6);
  } else if (E.emode == MODE_COMMAND) {
    appendStatusString("COMMAND", 7);
  }
}

void setMessageString(void) {
  if (E.emode == MODE_COMMAND) {
    appendMessageString(":", 1);
    appendMessageString(E.cmd.cmd_str, E.cmd.cmd_len);
  } else {
    appendMessageString("",0);
  }
}

void resetStatusString(void) {
  E.estat.stat_str[0] = '\0';
  E.estat.stat_len = 0;
}

void resetMessageString(void) {
  E.emsg.msg_str[0] = '\0';
  E.emsg.msg_len = 0;
}

void setCursor(void) {
  if (E.emode == MODE_COMMAND) {
    E.crsr_cmd_x = E.emsg.msg_len + 2;
    E.crsr_cmd_y = E.term_height-1;
    E.crsr_rndr_x = E.crsr_cmd_x + 1;
    E.crsr_rndr_y = E.crsr_cmd_y + 1;
  } else {
    int rx = 0;
    ERow *erow = (E.crsr_y >= E.num_row) ? NULL : &E.erow[E.crsr_y];

    if (erow) {
      for (int x = 0; x < E.crsr_x; x++) {
        if (erow->row_str[x] == '\t')
          rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
        rx++;
      }
    }

    E.crsr_rndr_x = rx - E.col_off + 1;
    E.crsr_rndr_y = E.crsr_y - E.row_off + 1;

    int row_len = erow ? erow->row_len : 0;
    if (E.crsr_rndr_x > row_len)
      E.crsr_rndr_x = row_len;

  }
}

void refreshScreen(void) {
  bufInit(&ob);

  // Updating window size for every refresh
  getWindowSize(&E.term_height, &E.term_width);

  // Cleaning the screen for the next refresh
  bufAppend(&ob, "\x1b[?25l", 6);
  bufAppend(&ob, "\x1b[H", 3);
  bufAppend(&ob, "\x1b[J", 3);

  // Drawing the editor screen
  editorScroll();
  drawEditorScreen(&ob);

  // Drawing the status and message bars
  setStatusString();
  setMessageString();
  drawStatusBar(&ob);
  drawMessageBar(&ob);

  bufAppend(&ob, "\x1b[?25h", 6);

  // Setting the cursor position on the editor
  setCursor();
  char crsr_pos[32]; 
  snprintf(crsr_pos, 32, "\x1b[%d;%dH", E.crsr_rndr_y, E.crsr_rndr_x);
  bufAppend(&ob, crsr_pos, strlen(crsr_pos));

  write(STDOUT_FILENO, ob.str, ob.p_str);

  resetStatusString();
  resetMessageString();
  bufFree(&ob);
}

