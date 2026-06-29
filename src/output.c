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
static int visualSelectionContains(int row_idx, int col_idx) {
  if (E.emode != MODE_VISUAL)
    return 0;

  int start_row = E.visual_anchor_y;
  int end_row = E.crsr_y;
  int start_col = E.visual_anchor_x;
  int end_col = E.crsr_x;

  if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
    int tmp_row = start_row;
    int tmp_col = start_col;
    start_row = end_row;
    start_col = end_col;
    end_row = tmp_row;
    end_col = tmp_col;
  }

  if (start_row < 0)
    start_row = 0;
  if (end_row < 0)
    end_row = 0;
  if (start_row >= E.num_row)
    start_row = (E.num_row > 0) ? E.num_row - 1 : 0;
  if (end_row >= E.num_row)
    end_row = (E.num_row > 0) ? E.num_row - 1 : 0;

  if (start_col < 0)
    start_col = 0;
  if (end_col < 0)
    end_col = 0;
  if (E.num_row > 0) {
    if (start_col > E.erow[start_row].row_len)
      start_col = E.erow[start_row].row_len;
    if (end_col > E.erow[end_row].row_len)
      end_col = E.erow[end_row].row_len;
  }

  if (row_idx < start_row || row_idx > end_row)
    return 0;
  if (row_idx == start_row && row_idx == end_row)
    return col_idx >= start_col && col_idx < end_col;
  if (row_idx == start_row)
    return col_idx >= start_col;
  if (row_idx == end_row)
    return col_idx < end_col;
  return 1;
}

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
      int display_col = 0;
      for (int col_idx = E.col_off; col_idx < E.erow[curr_row].row_len && display_col < E.term_width; col_idx++) {
        int is_selected = visualSelectionContains(curr_row, col_idx);
        if (is_selected)
          bufAppend(ob, "\x1b[7m", 4);

        if (E.erow[curr_row].row_str[col_idx] == '\t') {
          int tab_width = TAB_SIZE - (display_col % TAB_SIZE);
          for (int tab_idx = 0; tab_idx < tab_width && display_col < E.term_width; tab_idx++) {
            bufAppend(ob, " ", 1);
            display_col++;
          }
        } else {
          char ch[2] = { E.erow[curr_row].row_str[col_idx], '\0' };
          bufAppend(ob, ch, 1);
          display_col++;
        }

        if (is_selected)
          bufAppend(ob, "\x1b[m", 3);
      }
    }
    bufAppend(ob, "\x1b[K", 3);
    if (row < E.term_height - 2)
      bufAppend(ob, "\r\n", 2);
  }
}

void drawStatusBar(OBuf *ob) {        // Drawing the status bar
  int stat_str_crsr = 0;
  char stat_str_right[100];
  int right_len = snprintf(stat_str_right, 100, "%s %d - %d", 
                           (E.dirt_flag_pos || E.dirt_flag_neg) ? "modified, " : "",
                           E.crsr_y+1, E.crsr_x+1);

  bufAppend(ob, "\x1b[7m", 4);
  for (int stat_crsr = 0; stat_crsr < E.term_width - right_len - 1; stat_crsr++) {
    if (stat_crsr > 1 && (size_t)stat_crsr < E.estat.stat_len + 2) {
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

  for (unsigned int msg_crsr = 0; (int)msg_crsr < E.term_width; msg_crsr++) {
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
  int stat_len = snprintf(stat_str, 100, "%.20s - %d lines - ",
                     E.filename ? E.filename : "[No File]", E.num_row);
  appendStatusString(stat_str, stat_len);

  if (E.emode == MODE_NORMAL) {
    appendStatusString("NORMAL", 6);
  } else if (E.emode == MODE_INSERT) {
    appendStatusString("INSERT", 6);
  } else if (E.emode == MODE_COMMAND) {
    appendStatusString("COMMAND", 7);
  } else if (E.emode == MODE_VISUAL) {
    appendStatusString("VISUAL", 6);
  }
}

void setMessageString(void) {
  if (E.emode == MODE_COMMAND) {
    E.emsg.msg_time = 0;
  } else {
  }
}

void resetStatusString(void) {
  E.estat.stat_str[0] = '\0';
  E.estat.stat_len = 0;
}

void resetMessageString(void) {
  if (time(NULL) - E.emsg.msg_time > MSG_TIME) {
    E.emsg.msg_str[0] = '\0';
    E.emsg.msg_len = 0;
  }
}

void setCursorPosition(void) {
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

    int row_len = erow ? erow->rndr_len : 0;
    if (E.crsr_rndr_x > row_len)
      E.crsr_rndr_x = row_len + 1;
  }
}

void editorRefreshScreen(void) {
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

  // Setting the cursor style and position on the editor
  setCursorPosition();
  char crsr_pos[32]; 
  snprintf(crsr_pos, 32, "\x1b[%d;%dH", E.crsr_rndr_y, E.crsr_rndr_x);
  bufAppend(&ob, crsr_pos, strlen(crsr_pos));

  write(STDOUT_FILENO, ob.str, ob.p_str);

  resetStatusString();
  resetMessageString();
  bufFree(&ob);
}

