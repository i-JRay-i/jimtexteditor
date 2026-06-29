#include "term.h"

#define CTRL_KEY(key) (key & 0x1f)

struct termios term_conf_def;

/* Helper functions for terminal mode logic and editor lifecycle */
static void setInsertCursorStyle (void) {
  write(STDOUT_FILENO, "\x1b[5 q", 5);
  fflush(stdout);
}

static void setNormalCursorStyle (void) {
  write(STDOUT_FILENO, "\x1b[2 q", 5);
  fflush(stdout);
}

static void enterInsertMode (void) {
  E.emode = MODE_INSERT;
  setInsertCursorStyle();
}

static void leaveInsertMode (void) {
  E.emode = MODE_NORMAL;
  setNormalCursorStyle();
  if (E.dirt_flag_pos || E.dirt_flag_neg)
    bufferSaveEditorState();
}

static void enterVisualMode (void) {
  E.visual_anchor_x = E.crsr_x;
  E.visual_anchor_y = E.crsr_y;
  E.emode = MODE_VISUAL;
}

/* Kills the editor gracefully and exits the program */
void die (const char* err_msg) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(err_msg);
  disableRawMode();
  editorFreeEditor();
  exit(1);
}

/* Fallback mechanism to read the cursor crsr_position */
int getCursorPosition (int *rows, int *cols) { 
  char str_crsr[32];
  unsigned int i_str = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while (i_str < sizeof(str_crsr) - 1) {
    if (read(STDIN_FILENO, &str_crsr[i_str], 1) != 1)
      break;
    if (str_crsr[i_str] == 'R')
      break;
    i_str++;
  }
  str_crsr[i_str] = '\0';

  if (str_crsr[0] != '\x1b' || str_crsr[1] != '[')
    return -1;
  if (sscanf(&str_crsr[2], "%d;%d", rows, cols) != 2)
    return -1;
  return 0;
}

int getWindowSize (int *rows, int *cols) {
  struct winsize term_size;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &term_size) == -1 || term_size.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = term_size.ws_col;
    *rows = term_size.ws_row;
    return 0;
  }
}

/* Cursor movement and navigation helpers */
static void moveCursorSimple (ERow *erow, int key) {
  switch (key) {
    case MOVE_LEFT:
      if (E.crsr_x > 0)
        E.crsr_x--;
      break;
    case MOVE_RIGHT:
      if (erow && E.crsr_x < erow->row_len)
        E.crsr_x++;
      break;
    case MOVE_UP:
      if (E.crsr_y > 0)
        E.crsr_y--;
      break;
    case MOVE_DOWN:
      if (E.crsr_y < E.num_row-1)
        E.crsr_y++;
      break;
  }
}

static void moveCursorWordForward (ERow *erow) {
  while (erow) {
    if (erow->row_str[E.crsr_x] == ' ') {
      E.crsr_x++;
      if (erow->row_str[E.crsr_x] != ' ')
        break;
    } else if (erow->row_str[E.crsr_x] == '\0') {
      E.crsr_x = 0;
      E.crsr_y++;
      erow = (E.crsr_y < E.num_row) ? &E.erow[E.crsr_y] : NULL;
      if (!erow || erow->row_str[0] != ' ')
        break;
    } else {
      E.crsr_x++;
    }
  }
}

static void moveCursorWordForwardEnd (ERow *erow) {
  E.crsr_x++;
  while (erow) {
    if (erow->row_str[E.crsr_x] == ' ') {
      E.crsr_x++;
    } else if (erow->row_str[E.crsr_x] == '\0') {
      E.crsr_x = 0;
      E.crsr_y++;
      erow = (E.crsr_y < E.num_row) ? &E.erow[E.crsr_y] : NULL;
    } else {
      E.crsr_x++;
      if (erow->row_str[E.crsr_x] == ' ' || erow->row_str[E.crsr_x] == '\0') {
        E.crsr_x--;
        break;
      }
    }
  }
}

static void moveCursorWordBackward (ERow *erow) {
  if (!erow) {
    E.crsr_y--;
    erow = (E.crsr_y >= E.num_row) ? NULL : &E.erow[E.crsr_y];
    E.crsr_x = E.erow[E.crsr_y].row_len - 1;
    return;
  }
  E.crsr_x--;
  while (erow) {
    if (E.crsr_x < 0) {
      if (E.crsr_y == 0) {
        E.crsr_x = 0;
        break;
      }
      E.crsr_y--;
      erow = &E.erow[E.crsr_y];
      E.crsr_x = E.erow[E.crsr_y].row_len - 1;
    }
    if (erow->row_str[E.crsr_x] == ' ') {
      E.crsr_x--;
    } else {
      if (E.crsr_x == 0) {
        break;
      }
      E.crsr_x--;
      if (erow->row_str[E.crsr_x] == ' ') {
        E.crsr_x++;
        break;
      }
    }
  }
}

static void moveCursorTokenForward (ERow *erow) {
  while (erow) {
    if (erow->row_str[E.crsr_x] == ' ') {
      E.crsr_x++;
      if (erow->row_str[E.crsr_x] != ' ')
        break;
    } else if (erow->row_str[E.crsr_x] == '\0') {
      E.crsr_x = 0;
      E.crsr_y++;
      erow = (E.crsr_y < E.num_row) ? &E.erow[E.crsr_y] : NULL;
      if (!erow || erow->row_str[0] != ' ')
        break;
    } else if ((isalnum(erow->row_str[E.crsr_x])) || (erow->row_str[E.crsr_x]) == '_') {
      E.crsr_x++;
      if ((!isalnum(erow->row_str[E.crsr_x])) && (erow->row_str[E.crsr_x]) != '_' && (erow->row_str[E.crsr_x] != ' ')) {
        break;
      }
    } else {
      E.crsr_x++;
      if ((isalnum(erow->row_str[E.crsr_x]) || erow->row_str[E.crsr_x] == '_') && erow->row_str[E.crsr_x] != ' ') {
        break;
      }
    }
  }
}

static void moveCursorTokenForwardEnd (ERow *erow) {
  E.crsr_x++;
  while (erow) {
    if (erow->row_str[E.crsr_x] == ' ') {
      E.crsr_x++;
    } else if (erow->row_str[E.crsr_x] == '\0') {
      E.crsr_x = 0;
      E.crsr_y++;
      erow = (E.crsr_y < E.num_row) ? &E.erow[E.crsr_y] : NULL;
      if (!erow || erow->row_str[0] == '\0')
        break;
    } else if ((isalnum(erow->row_str[E.crsr_x])) || (erow->row_str[E.crsr_x]) == '_') {
      E.crsr_x++;
      if ((!isalnum(erow->row_str[E.crsr_x]) && erow->row_str[E.crsr_x] != '_') || erow->row_str[E.crsr_x] == ' ') {
        E.crsr_x--;
        break;
      }
    } else {
      E.crsr_x++;
      if ((isalnum(erow->row_str[E.crsr_x]) || erow->row_str[E.crsr_x] == '_') || erow->row_str[E.crsr_x] == ' ') {
        E.crsr_x--;
        break;
      }
    }
  }
}

static void moveCursorTokenBackward (ERow *erow) {
  if (!erow) {
    E.crsr_y--;
    erow = (E.crsr_y >= E.num_row) ? NULL : &E.erow[E.crsr_y];
    return;
  }
  E.crsr_x--;
  while (erow) {
    if (E.crsr_x < 0) {
      if (E.crsr_y == 0) {
        E.crsr_x = 0;
        break;
      }
      E.crsr_y--;
      erow = &E.erow[E.crsr_y];
      E.crsr_x = E.erow[E.crsr_y].row_len - 1;
    }
    if (erow->row_str[E.crsr_x] == ' ') {
      E.crsr_x--;
    } else if ((isalnum(erow->row_str[E.crsr_x])) || (erow->row_str[E.crsr_x]) == '_') {
      E.crsr_x--;
      if ((!isalnum(erow->row_str[E.crsr_x]) && erow->row_str[E.crsr_x] != '_') || erow->row_str[E.crsr_x] == ' ') {
        E.crsr_x++;
        break;
      }
    } else {
      E.crsr_x--;
      if ((isalnum(erow->row_str[E.crsr_x]) || (erow->row_str[E.crsr_x]) == '_') || erow->row_str[E.crsr_x] == ' ') {
        E.crsr_x++;
        break;
      }
    }
  }
}

static void moveCursorPageJump (int key) {
  switch (key) {
    case HALF_PAGE_UP:
      E.crsr_y -= HALF_PAGE_SIZE;
      if (E.crsr_y < 0)
        E.crsr_y = 0;
      break;
    case HALF_PAGE_DOWN:
      E.crsr_y += HALF_PAGE_SIZE;
      if (E.crsr_y > E.num_row)
        E.crsr_y = E.num_row;
      break;
  }
}

static void moveCursorLineJump (int key) {
  switch (key) {
    case EOF_KEY:
      E.crsr_y = E.num_row-1;
      E.crsr_x = 0;
      break;
    case INIT_FILE_KEY:
      E.crsr_y = 0;
      E.crsr_x = 0;
      break;
    case EOL_KEY:
      E.crsr_x = E.erow[E.crsr_y].row_len;
      break;
    case INIT_LINE_KEY:
      E.crsr_x = 0;
      break;
  }
}

void moveCursor (int key) {
  ERow *erow = (E.crsr_y < E.num_row) ? &E.erow[E.crsr_y] : NULL;
  switch (key) {
    case MOVE_LEFT:
    case MOVE_RIGHT:
    case MOVE_UP:
    case MOVE_DOWN:
      moveCursorSimple(erow, key);
      break;
    case MOVE_WORD_FORWARD:
      moveCursorWordForward(erow);
      break;
    case MOVE_WORD_FORWARD_END:
      moveCursorWordForwardEnd(erow);
      break;
    case MOVE_WORD_BACKWARD:
      moveCursorWordBackward(erow);
      break;
    case MOVE_TOKEN_FORWARD:
      moveCursorTokenForward(erow);
      break;
    case MOVE_TOKEN_FORWARD_END:
      moveCursorTokenForwardEnd(erow);
      break;
    case MOVE_TOKEN_BACKWARD:
      moveCursorTokenBackward(erow);
      break;
    case HALF_PAGE_UP:
    case HALF_PAGE_DOWN:
      moveCursorPageJump(key);
      break;
    case EOF_KEY:
    case INIT_FILE_KEY:
    case EOL_KEY:
    case INIT_LINE_KEY:
      moveCursorLineJump(key);
      break;
  }
}

void moveCursorCommand (int key) {
  switch (key) {
    case MOVE_LEFT:
      if (E.crsr_cmd_x > 0)
        E.crsr_cmd_x--;
      break;
    case MOVE_RIGHT:
      if (E.crsr_cmd_x < E.term_width)
        E.crsr_cmd_x++;
      break;
  }
}

int readKey (void) {
  int key = 0;
  if ((read(STDIN_FILENO, &key, 1)) == -1)
    die("read");
  return key;
}

/* Used for debugging */
void printKey (void) {
  int key = readKey();
  printf("%d ('%c')\r\n", key, key);
  write(STDOUT_FILENO, &key, 1);
}

int mapKeyNormal (int key) {
  switch (key){
    case CTRL_KEY('q'): return EXIT_KEY;
    case 'h': return MOVE_LEFT;
    case 'j': return MOVE_DOWN;
    case 'k': return MOVE_UP;
    case 'l': return MOVE_RIGHT;
    case 'W': return MOVE_WORD_FORWARD;
    case 'E': return MOVE_WORD_FORWARD_END;
    case 'B': return MOVE_WORD_BACKWARD;
    case 'w': return MOVE_TOKEN_FORWARD;
    case 'e': return MOVE_TOKEN_FORWARD_END;
    case 'b': return MOVE_TOKEN_BACKWARD;
    case 'G': return EOF_KEY;
    case '$': return EOL_KEY;
    case '0': return INIT_LINE_KEY;
    case 'i': return INSERT_KEY;
    case 'a': return INSERT_NEXT_KEY;
    case 'o': return INSERT_LINE_NEXT;
    case 'O': return INSERT_LINE_PREV;
    case 'v': return VISUAL_MODE_KEY;
    case ':': return COMMAND_KEY;
    case CTRL_KEY('u'): return HALF_PAGE_UP;
    case CTRL_KEY('d'): return HALF_PAGE_DOWN;
    case '/': return SEARCH_KEY;
    case 'n': return SEARCH_FORWARD;
    case 'N': return SEARCH_BACKWARD;
    case 'd': return DELETE_KEY;
    case 'y': return COPY_KEY;
    case 'p': return PASTE_KEY;
    case 'P': return PASTE_AFTER_KEY;
    case 'u': return UNDO_KEY;
    case CTRL_KEY('r'): return REDO_KEY;
    default: return key;
  }
}

/* Normal-mode key handling */
void processNormal (int key) {
   switch (key) {
    case EXIT_KEY:
      editorExitEditor();
      break;
    case INSERT_KEY:
      enterInsertMode();
      break;
    case INSERT_NEXT_KEY:
      if (E.erow && E.crsr_x < E.erow[E.crsr_y].row_len)
        E.crsr_x++;
      enterInsertMode();
      break;
    case INSERT_LINE_PREV:
      E.crsr_x = 0;
      erowInsertRow();
      E.crsr_y--;
      enterInsertMode();
      break;
    case INSERT_LINE_NEXT:
      E.crsr_x = 0;
      E.crsr_y++;
      erowInsertRow();
      E.crsr_y--;
      enterInsertMode();
      break;
    case COMMAND_KEY:
      E.emode = MODE_COMMAND;
      break;
    case VISUAL_MODE_KEY:
      enterVisualMode();
      break;
    case SEARCH_KEY:
      E.emode = MODE_COMMAND;
      searchPrompt();
      break;
    case SEARCH_FORWARD:
      E.srch.srch_match_idx++;
      if (E.srch.srch_match_idx >= E.srch.srch_match_num)
        E.srch.srch_match_idx = 0;
      searchQuery();
      break;
    case SEARCH_BACKWARD:
      E.srch.srch_match_idx--;
      if (E.srch.srch_match_idx >= E.srch.srch_match_num)
        E.srch.srch_match_idx = E.srch.srch_match_num-1;
      searchQuery();
      break;
    case DELETE_KEY:
      actionDelete();
      bufferSaveEditorState();
      break;
    case COPY_KEY:
      actionCopy();
      break;
    case PASTE_AFTER_KEY:
      if (E.eclip && E.eclip->clip_type == CLIPBOARD_LINE) {
        if (E.crsr_y < E.num_row)
          E.crsr_y++;
      } else {
        moveCursor(MOVE_RIGHT);
      }
      actionPaste();
      bufferSaveEditorState();
      break;
    case PASTE_KEY:
      actionPaste();
      bufferSaveEditorState();
      break;
    case UNDO_KEY:
      bufferEditorUndo();
      break;
    case REDO_KEY:
      bufferEditorRedo();
      break;
    case HALF_PAGE_UP:
    case HALF_PAGE_DOWN:
    case MOVE_LEFT:
    case MOVE_DOWN:
    case MOVE_UP:
    case MOVE_RIGHT:
    case MOVE_WORD_FORWARD:
    case MOVE_WORD_FORWARD_END:
    case MOVE_WORD_BACKWARD:
    case MOVE_TOKEN_FORWARD:
    case MOVE_TOKEN_FORWARD_END:
    case MOVE_TOKEN_BACKWARD:
    case EOF_KEY:
      moveCursor(key);
      break;
  }
}

int mapKeyInsert (int key) {
  switch (key) {
    case CTRL_KEY('q'): return EXIT_KEY;
    case '\r': return NEWLINE_KEY;
    case 127: return ERASE_LEFT_KEY;
    case '\x1b': 
      {
        char escseq[8];
        if (read(STDIN_FILENO, &escseq[0], 1) != 1) return NORMAL_MODE_KEY;
        if (read(STDIN_FILENO, &escseq[1], 1) != 1) return NORMAL_MODE_KEY;

        if (escseq[0] == '[') {
          if (read(STDIN_FILENO, &escseq[2], 1) != 1) return NORMAL_MODE_KEY;
          if (escseq[2] == '~') {
            switch (escseq[1]) {
              case '3': return ERASE_RIGHT_KEY;
            }
          } else {
            switch (escseq[1]) {
              case 'A': return MOVE_UP;
              case 'B': return MOVE_DOWN;
              case 'C': return MOVE_RIGHT;
              case 'D': return MOVE_LEFT;
            }
          }
        }
        return NORMAL_MODE_KEY;
      }
    default: return key;
  }
}

/*  */
static void normalizeVisualSelection (int *row_start, int *row_end, int *col_start, int *col_end) {
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

  *row_start = start_row;
  *row_end = end_row;
  *col_start = start_col;
  *col_end = end_col;
}

/* Implements the VISUAL mode key handling logic */
void processVisual (int key) {
  switch (key) {
    case NORMAL_MODE_KEY:
    case VISUAL_MODE_KEY:
    case '\x1b':
      {
        int row_start = 0, row_end = 0;
        int col_start = 0, col_end = 0;
        normalizeVisualSelection(&row_start, &row_end, &col_start, &col_end);
      }
      E.emode = MODE_NORMAL;
      break;
    case 'y':
      {
        int row_start = 0, row_end = 0;
        int col_start = 0, col_end = 0;
        normalizeVisualSelection(&row_start, &row_end, &col_start, &col_end);
        bufferCopyToEClip(row_start, row_end, col_start, col_end);
        E.emode = MODE_NORMAL;
        setMessage("Yanked %d char(s).", (row_start == row_end) ? (col_end - col_start) : 0);
      }
      break;
    case MOVE_LEFT:
    case MOVE_DOWN:
    case MOVE_UP:
    case MOVE_RIGHT:
    case MOVE_WORD_FORWARD:
    case MOVE_WORD_FORWARD_END:
    case MOVE_WORD_BACKWARD:
    case MOVE_TOKEN_FORWARD:
    case MOVE_TOKEN_FORWARD_END:
    case MOVE_TOKEN_BACKWARD:
    case EOF_KEY:
      moveCursor(key);
      break;
    default:
      break;
  }
}

/* Implements the INSERT mode key handling logic */
void processInsert (int key) {
   switch (key) {
    case EXIT_KEY:
      editorExitEditor();
      break;
    case NORMAL_MODE_KEY:
      leaveInsertMode();
      break;
    case NEWLINE_KEY:
      erowInsertRow();
      break;
    case ERASE_LEFT_KEY:
    case ERASE_RIGHT_KEY:
      if (key == ERASE_RIGHT_KEY)
        moveCursor(MOVE_RIGHT);
      deleteChar();
      break;
    case MOVE_LEFT:
    case MOVE_DOWN:
    case MOVE_UP:
    case MOVE_RIGHT:
      moveCursor(key);
      break;
    case '\0':
      break;
    default:
      insertChar(key);
      break;
  }
}

int mapKeyCommand (int key) {
  switch (key){
    case '\x1b': 
      {
      char esc_seq[6];
        if (read(STDIN_FILENO, &esc_seq[0], 1) != 1)
          return NORMAL_MODE_KEY;
        if (read(STDIN_FILENO, &esc_seq[1], 1) != 1)
          return NORMAL_MODE_KEY;
        if (esc_seq[0] == '[') {
          switch (esc_seq[1]) {
            case 'A': return MOVE_UP;
            case 'B': return MOVE_DOWN;
            case 'C': return MOVE_RIGHT;
            case 'D': return MOVE_LEFT;
          }
        }
      }
      return NORMAL_MODE_KEY;
    case '\r': return ENTER_COMMAND_KEY;
    case 127: return ERASE_LEFT_KEY;
    default: return key;
  }
}

/* Command-mode and prompt handling */
void enterCommand (void) {
  if (!strcmp(E.cmd.cmd_str, "q")) {
    if ((E.dirt_flag_pos || E.dirt_flag_neg)) {
      setMessage("Warning: File has unsaved changes.");
      return;
    }
    editorExitEditor();
  } else if (!strcmp(E.cmd.cmd_str, "w")) {
    saveFile();
  } else if (!strcmp(E.cmd.cmd_str, "wq")) {
    saveFile();
    editorExitEditor();
  } else if (!strcmp(E.cmd.cmd_str, "q!")) {
    editorExitEditor();
  } else if (!strcmp(E.cmd.cmd_str, "help")) {
    setMessage(HELP_MSG);
  } else if (E.cmd.cmd_str[0] == '!') {
    system(&E.cmd.cmd_str[1]);
  } else {
    setMessage("Invalid command.");
  }
}

void clearCommand (void) {
  for (size_t cmd_idx = 0; cmd_idx < E.cmd.cmd_len; cmd_idx++)
    E.cmd.cmd_str[cmd_idx] = 0;
  E.cmd.cmd_len = 0;
  E.cmd.cmd_str[0] = '\0';
}

/* Implements the COMMAND mode key handling logic */
void processCommand (int key) {
  switch (key) {
    case '\0':
      break;
    case NORMAL_MODE_KEY:
      clearCommand();
      E.emode = MODE_NORMAL;
      break;
    case ENTER_COMMAND_KEY:
      enterCommand();
      E.cmd.cmd_len = 0;
      E.cmd.cmd_str[0] = '\0';
      E.emode = MODE_NORMAL;
      break;
    case ERASE_LEFT_KEY:
      if (E.cmd.cmd_len > 0) {
        E.crsr_cmd_x--;
        E.cmd.cmd_len--;
        E.cmd.cmd_str[E.cmd.cmd_len] = '\0';
      }
      break;
    case MOVE_UP:
    case MOVE_DOWN:
    case MOVE_LEFT:
    case MOVE_RIGHT:
      moveCursorCommand(key);
      break;
    default:
      E.cmd.cmd_str[E.cmd.cmd_len] = key;
      E.cmd.cmd_len++;
      E.cmd.cmd_str[E.cmd.cmd_len] = '\0';
      break;
  }
}

void findQuery (void) {
  int row_idx = 0;
  int col_idx = 0;

  E.srch.srch_match_num = 0;
  E.srch.srch_match_idx = 0;

  while (row_idx < E.num_row) {
    ERow *curr_row = &E.erow[row_idx];
    char *str_match = strstr(&(curr_row->row_str[col_idx]), E.srch.srch_str);

    if (str_match) {
      col_idx = (int)(str_match - curr_row->row_str);
      E.srch.srch_match_num++;

      E.srch.srch_match_x = realloc(E.srch.srch_match_x, sizeof(int) * (E.srch.srch_match_num));
      E.srch.srch_match_y = realloc(E.srch.srch_match_y, sizeof(int) * (E.srch.srch_match_num));
      E.srch.srch_match_x[E.srch.srch_match_num-1] = col_idx;  
      E.srch.srch_match_y[E.srch.srch_match_num-1] = row_idx;

      if (E.srch.srch_match_y[E.srch.srch_match_num-1] <= E.crsr_y)
        E.srch.srch_match_idx = E.srch.srch_match_num;

      col_idx++;
      continue;
    }

    row_idx++;
    col_idx = 0;
  }
}

void searchQuery (void) {
  E.crsr_x = E.srch.srch_match_x[E.srch.srch_match_idx];
  E.crsr_y = E.srch.srch_match_y[E.srch.srch_match_idx];
}

/* Search-mode and search prompt handling */
void processSearch (int key) {
  switch (key) {
    case '\0':
      break;
    case NORMAL_MODE_KEY:
      clearCommand();
      E.emode = MODE_NORMAL;
      break;
    case ENTER_COMMAND_KEY:
      findQuery();
      searchQuery();
      E.emode = MODE_NORMAL;
      break;
    case ERASE_LEFT_KEY:
      if (E.srch.srch_len > 0) {
        E.crsr_cmd_x--;
        E.srch.srch_len -= 1;
        E.srch.srch_str[E.srch.srch_len] = '\0';
      }
      break;
    case MOVE_UP:
    case MOVE_DOWN:
      break;
    case MOVE_LEFT:
    case MOVE_RIGHT:
      moveCursorCommand(key);
      break;
    default:
      E.srch.srch_str[E.srch.srch_len] = key;
      E.srch.srch_len += 1;
      E.srch.srch_str[E.srch.srch_len] = '\0';
      break;
  }
}

void searchPrompt (void) {
  E.srch.srch_size = 512;
  E.srch.srch_str = realloc(E.srch.srch_str, E.srch.srch_size);
  E.srch.srch_str[0] = '\0';
  E.srch.srch_len = 0;

  while (1) {
    if (E.srch.srch_len > E.srch.srch_size) {
      E.srch.srch_size += 512;
      E.srch.srch_str = realloc(E.srch.srch_str, E.srch.srch_size);
    }

    int key = readKey();
    key = mapKeyCommand(key);
    processSearch(key);

    if ((key == ENTER_COMMAND_KEY) || (key == NORMAL_MODE_KEY)) {
      break;
    }

    appendMessageString("/", 1);
    appendMessageString(E.srch.srch_str, E.srch.srch_len);
    editorRefreshScreen();
  }
}

/* Copy, delete, and action helpers */
/* Helpers for copy operations */
static void copyLineSelection (int row_idx) {
  if (row_idx < E.num_row) {
    bufferCopyToEClip(row_idx, row_idx, 0, E.erow[row_idx].row_len);
    setMessage("Yanked %d line(s).", E.eclip->num_row_eclip);
  }
}

static void copyWordForward (int row_idx, int col_idx) {
  if (row_idx < E.num_row) {
    char *line = E.erow[row_idx].row_str;
    int line_len = E.erow[row_idx].row_len;
    int end = col_idx;

    if (end < line_len) {
      if (isalnum(line[end])) {
        while (end < line_len && isalnum(line[end]))
          end++;
      } else {
        while (end < line_len && !isalnum(line[end]))
          end++;
      }
      while (end < line_len && line[end] == ' ')
        end++;
    }

    if (end > col_idx) {
      bufferCopyToEClip(row_idx, row_idx, col_idx, end);
      setMessage("Yanked %d chars.", end - col_idx);
    }
  }
}

static void copyWordEnd (int row_idx, int col_idx) {
  if (row_idx < E.num_row) {
    char *line = E.erow[row_idx].row_str;
    int line_len = E.erow[row_idx].row_len;
    int end = col_idx;

    if (end < line_len) {
      if (line[end] == ' ') {
        while (end < line_len && line[end] == ' ')
          end++;
      }
      if (end < line_len) {
        if (isalnum(line[end])) {
          while (end < line_len && isalnum(line[end]))
            end++;
        } else {
          while (end < line_len && !isalnum(line[end]) && line[end] != ' ')
            end++;
        }
      }
    }

    if (end > col_idx) {
      bufferCopyToEClip(row_idx, row_idx, col_idx, end);
      setMessage("Yanked %d chars.", end - col_idx);
    }
  }
}

static void copyWordBackward (int row_idx, int col_idx) {
  if (row_idx < E.num_row) {
    char *line = E.erow[row_idx].row_str;
    int start = col_idx;

    if (start > 0) {
      start--;
      while (start > 0 && line[start] == ' ')
        start--;

      if (isalnum(line[start])) {
        while (start > 0 && isalnum(line[start-1]))
          start--;
      } else {
        while (start > 0 && !isalnum(line[start-1]) && line[start-1] != ' ')
          start--;
      }
    }

    if (start < col_idx) {
      bufferCopyToEClip(row_idx, row_idx, start, col_idx);
      setMessage("Yanked %d chars.", col_idx - start);
    }
  }
}

static void copyWORDForward (int row_idx, int col_idx) {
  if (row_idx < E.num_row) {
    char *line = E.erow[row_idx].row_str;
    int line_len = E.erow[row_idx].row_len;
    int end = col_idx;

    if (end < line_len) {
      while (end < line_len && line[end] != ' ')
        end++;
      while (end < line_len && line[end] == ' ')
        end++;
    }

    if (end > col_idx) {
      bufferCopyToEClip(row_idx, row_idx, col_idx, end);
      setMessage("Yanked %d chars.", end - col_idx);
    }
  }
}

static void copyWORDEnd (int row_idx, int col_idx) {
  if (row_idx < E.num_row) {
    char *line = E.erow[row_idx].row_str;
    int line_len = E.erow[row_idx].row_len;
    int end = col_idx;

    if (end < line_len) {
      while (end < line_len && line[end] == ' ')
        end++;
      while (end < line_len && line[end] != ' ')
        end++;
    }

    if (end > col_idx) {
      bufferCopyToEClip(row_idx, row_idx, col_idx, end);
      setMessage("Yanked %d chars.", end - col_idx);
    }
  }
}

static void copyWORDBackward (int row_idx, int col_idx) {
  if (row_idx < E.num_row) {
    char *line = E.erow[row_idx].row_str;
    int start = col_idx;

    if (start > 0) {
      start--;
      while (start > 0 && line[start] == ' ')
        start--;
      while (start > 0 && line[start-1] != ' ')
        start--;
    }

    if (start < col_idx) {
      bufferCopyToEClip(row_idx, row_idx, start, col_idx);
      setMessage("Yanked %d chars.", col_idx - start);
    }
  }
}

void processCopy (void) {
  int key = readKey();
  while (key == 0) {
    key = readKey();
  }

  int curr_x = E.crsr_x;
  int curr_y = E.crsr_y;

  switch (key) {
    case '\x1b':
      break;
    case 'y':
      copyLineSelection(curr_y);
      break;
    case 'w':
      copyWordForward(curr_y, curr_x);
      break;
    case 'e':
      copyWordEnd(curr_y, curr_x);
      break;
    case 'b':
      copyWordBackward(curr_y, curr_x);
      break;
    case 'W':
      copyWORDForward(curr_y, curr_x);
      break;
    case 'E':
      copyWORDEnd(curr_y, curr_x);
      break;
    case 'B':
      copyWORDBackward(curr_y, curr_x);
      break;
    default:
      break;
  }
  return;
}

/* Helpers for delete operations */
static void deleteLineSelection (int curr_x, int curr_y) {
  E.crsr_x = E.erow[E.crsr_y].row_len;
  while (curr_y == E.crsr_y) {
    deleteChar();
  }
  E.crsr_x = curr_x; E.crsr_y = curr_y;
  setMessage("Deleted 1 line");
}

static void deleteToLineStart (void) {
  int ch = 0;
  while (E.crsr_x > 0) {
    deleteChar();
    ch++;
  }
  setMessage("Deleted %d chars.", ch);
}

static void deleteToLineEnd (void) {
  int ch = 0;
  while (E.crsr_x < E.erow[E.crsr_y].row_len) {
    deleteChar();
    E.crsr_x++;
    ch++;
  }
  deleteChar();
  ch++;
  setMessage("Deleted %d chars.", ch);
}

static void deleteWordForward (void) {
  int ch = 0;
  if (isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
    while (isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
      E.crsr_x++;
      deleteChar();
      ch++;
    }
  } else if (!isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
    while (!isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
      E.crsr_x++;
      deleteChar();
      ch++;
    }
  }
  while (E.erow[E.crsr_y].row_str[E.crsr_x] == ' ') {
    E.crsr_x++;
    deleteChar();
    ch++;
  }
  setMessage("Deleted %d chars.", ch);
}

static void deleteWordEnd (void) {
  int ch = 0;
  while (E.erow[E.crsr_y].row_str[E.crsr_x] == ' ') {
    E.crsr_x++;
    deleteChar();
    ch++;
  }
  if (isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
    while (isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
      E.crsr_x++;
      deleteChar();
      ch++;
    }
  } else if (!isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
    while (!isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
      E.crsr_x++;
      deleteChar();
      ch++;
    }
  }
  setMessage("Deleted %d chars.", ch);
}

static void deleteWordBackward (void) {
  if (E.crsr_x == 0 && E.crsr_y == 0)
    return;
  E.crsr_x--;
  while (E.erow[E.crsr_y].row_str[E.crsr_x] == ' ') {
    E.crsr_x++;
    deleteChar();
    E.crsr_x--;
  }
  if (isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
    while (isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
      E.crsr_x++;
      deleteChar();
      E.crsr_x--;
    }
  } else if (!isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
    while (!isalnum(E.erow[E.crsr_y].row_str[E.crsr_x])) {
      E.crsr_x++;
      deleteChar();
      E.crsr_x--;
    }
  }
  E.crsr_x++;
  setMessage("Deleted 1 word before.");
}

static void deleteWORDForward (void) {
  while (E.erow[E.crsr_y].row_str[E.crsr_x] != ' ' && E.erow[E.crsr_y].row_str[E.crsr_x] != '\0') {
    E.crsr_x++;
    deleteChar();
  } while (E.erow[E.crsr_y].row_str[E.crsr_x] == ' ') {
    E.crsr_x++;
    deleteChar();
  }
  setMessage("Deleted 1 WORD.");
}

static void deleteWORDEnd (void) {
  while (E.erow[E.crsr_y].row_str[E.crsr_x] == ' ') {
    E.crsr_x++;
    deleteChar();
  } while (E.erow[E.crsr_y].row_str[E.crsr_x] != ' '&& E.erow[E.crsr_y].row_str[E.crsr_x] != '\0') {
    E.crsr_x++;
    deleteChar();
  }
  setMessage("Deleted 1 WORD.");
}

static void deleteWORDBackward (void) {
  if (E.crsr_x == 0 && E.crsr_y == 0)
    return;
  E.crsr_x--;
  while (E.erow[E.crsr_y].row_str[E.crsr_x] == ' ') {
    E.crsr_x++;
    deleteChar();
    E.crsr_x--;
  } while (E.erow[E.crsr_y].row_str[E.crsr_x] != ' ' && E.erow[E.crsr_y].row_str[E.crsr_x] != '\0') {
    E.crsr_x++;
    deleteChar();
    E.crsr_x--;
  }
  E.crsr_x++;
  setMessage("Deleted 1 WORD before.");
}

void processDelete (void) {
  int key = readKey();
  while (key == 0) {
    key = readKey();
  }

  int curr_x = E.crsr_x;
  int curr_y = E.crsr_y;
  switch (key) {
    case '\x1b': 
      break;
    case 'd':
      copyLineSelection(curr_y); 
      deleteLineSelection(curr_x, curr_y);
      break;
    case '0':
      deleteToLineStart();
      break;
    case '$':
      deleteToLineEnd();
      break;
    case 'w':
      copyWordForward(curr_y, curr_x);
      deleteWordForward();
      break;
    case 'e':
      deleteWordEnd();
      break;
    case 'b':
      deleteWordBackward();
      break;
    case 'W':
      deleteWORDForward();
      break;
    case 'E':
      deleteWORDEnd();
      break;
    case 'B':
      deleteWORDBackward();
      break;
    default:
      break;
  }
  return;
}

/* Action wrappers for editor commands */
void actionCopy (void) {
  write(STDOUT_FILENO, "\x1b[3 q", 5);
  processCopy();
  write(STDOUT_FILENO, "\x1b[2 q", 5);
  return;
}

void actionPaste (void) {
  write(STDOUT_FILENO, "\x1b[3 q", 5);
  if (!E.eclip || E.eclip->num_row_eclip == 0) {
    write(STDOUT_FILENO, "\x1b[2 q", 5);
    setMessage("Clipboard empty.");
    return;
  }
  bufferPasteEClip();
  write(STDOUT_FILENO, "\x1b[2 q", 5);
  setMessage("Pasted %d line(s).", E.eclip->num_row_eclip);
  return;
}

void actionDelete (void) {
  write(STDOUT_FILENO, "\x1b[3 q", 5);
  processDelete();
  write(STDOUT_FILENO, "\x1b[2 q", 5);
  return;
}

void commandPrompt (void) {
  while (1) {
    int key = readKey();
    key = mapKeyCommand(key);
    processCommand(key);
    if ((key == ENTER_COMMAND_KEY) || (key == NORMAL_MODE_KEY))
      break;

    appendMessageString(":", 1);
    appendMessageString(E.cmd.cmd_str, E.cmd.cmd_len);
    editorRefreshScreen();
  }
}

void visionPrompt (void) {
  return;
}

void editorProcessKey (void) {
  int key = readKey();
  if (E.emode == MODE_NORMAL) {
    key = mapKeyNormal(key);
    processNormal(key);
  } else if (E.emode == MODE_INSERT) {
    key = mapKeyInsert(key);
    processInsert(key);
  } else if (E.emode == MODE_COMMAND) {
    commandPrompt();
  } else if (E.emode == MODE_VISUAL) {
    key = mapKeyNormal(key);
    processVisual(key);
  } else {
    return;
  }
}

/* Status/message buffers and editor state helpers */
void appendStatusString (char *str, unsigned int str_len) {
  if (E.estat.stat_len + str_len > E.estat.stat_size) {
    E.estat.stat_size += 512;
    E.estat.stat_str = realloc(E.estat.stat_str, E.estat.stat_size);
  }

  memcpy(&E.estat.stat_str[E.estat.stat_len], str, str_len);
  E.estat.stat_str[E.estat.stat_len+str_len] = '\0';
  E.estat.stat_len = strlen(E.estat.stat_str);
}

void appendMessageString (char *str, unsigned int str_len) {
  if (E.emsg.msg_len + str_len > E.emsg.msg_size) {
    E.emsg.msg_size += 512;
    E.emsg.msg_str = realloc(E.emsg.msg_str, E.emsg.msg_size);
  }

  memcpy(&E.emsg.msg_str[E.emsg.msg_len], str, str_len);
  E.emsg.msg_str[E.emsg.msg_len+str_len] = '\0';
  E.emsg.msg_len = strlen(E.emsg.msg_str);
}

void setMessage (const char *fmt, ...) {
  E.emsg.msg_len = 0;
  E.emsg.msg_size = 0;
  free(E.emsg.msg_str);

  char msg_buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg_buf, 256, fmt, ap);
  va_end(ap);
  appendMessageString(msg_buf, strlen(msg_buf));
  E.emsg.msg_time = time(NULL);
}

/* Retrieving the terminal back to the default configuration */
void disableRawMode (void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_conf_def) == -1)
    die("tcsetattr");
}

/* Changing the terminal to the "raw" mode. */
void enableRawMode (void) {
  if (tcgetattr(STDIN_FILENO, &term_conf_def) == -1)
    die("tcgetattr");  

  E.term_conf_editor = term_conf_def;

  E.term_conf_editor.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  E.term_conf_editor.c_oflag &= ~(OPOST);
  E.term_conf_editor.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  E.term_conf_editor.c_cflag &= ~(CSIZE | PARENB);
  E.term_conf_editor.c_cflag |= (CS8);
  E.term_conf_editor.c_cc[VMIN] = 0;
  E.term_conf_editor.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.term_conf_editor) == -1)
    die("tcsetattr");
}

/* Initializing the Editor object. */
void editorInitEditor (void) {
  enableRawMode();
  getWindowSize(&E.term_height, &E.term_width);

  E.crsr_x = 0; E.crsr_y = 0;
  E.crsr_cmd_x = 0; E.crsr_cmd_y = 0;
  E.crsr_rndr_x = 0; E.crsr_rndr_y = 0;
  E.visual_anchor_x = 0; E.visual_anchor_y = 0;
  E.row_off = 0; E.col_off = 0;
  E.dirt_flag_pos = 0; E.dirt_flag_neg = 0;
  E.emode = MODE_NORMAL;

  // Initially there is no rows to draw
  E.erow = NULL;
  E.num_row = 0;

  // Setting up the history buffer 
  E.ebuff = malloc(sizeof(EHBuff));
  E.ebuff->hbuff_len = 0;
  E.ebuff->hbuff_idx = 0;
  E.ebuff->erow_hbuff = malloc(sizeof(ERow *) * HIST_BUFF_STACK_SIZE);
  E.ebuff->crsr_x_hbuff = malloc(sizeof(int) * HIST_BUFF_STACK_SIZE);
  E.ebuff->crsr_y_hbuff = malloc(sizeof(int) * HIST_BUFF_STACK_SIZE);
  E.ebuff->num_row_hbuff = malloc(sizeof(int) * HIST_BUFF_STACK_SIZE);
  for (int idx = 0; idx < HIST_BUFF_STACK_SIZE; idx++) {
    E.ebuff->crsr_x_hbuff[idx] = 0;
    E.ebuff->crsr_y_hbuff[idx] = 0;
    E.ebuff->num_row_hbuff[idx] = 0;
    E.ebuff->erow_hbuff[idx] = NULL;
  }

  // Initializing the status message buffer
  E.estat.stat_size = 512;
  E.estat.stat_str = malloc(E.estat.stat_size);
  E.estat.stat_str[0] = '\0';
  E.estat.stat_len = 0;

  // Initializing the editor message buffer
  E.emsg.msg_size = 512;
  E.emsg.msg_str = malloc(E.emsg.msg_size);
  E.emsg.msg_str[0] = '\0';
  E.emsg.msg_len = 0;
  E.emsg.msg_time = 0;

  // Initializing the command buffer
  E.cmd.cmd_size = 512;
  E.cmd.cmd_str = malloc(E.cmd.cmd_size);
  E.cmd.cmd_str[0] = '\0';
  E.cmd.cmd_len = 0;

  // Initializing the search buffer
  E.srch.srch_size = 512;
  E.srch.srch_str = malloc(E.srch.srch_size);
  E.srch.srch_str[0] = '\0';
  E.srch.srch_len = 0;
  E.srch.srch_match_num = 0;
  E.srch.srch_match_idx = 0;
  E.srch.srch_match_x = NULL;
  E.srch.srch_match_y = NULL;

  // Initializing the clipboard buffer
  E.eclip = malloc(sizeof(EClip));
  E.eclip->num_row_eclip = 0;
  E.eclip->eclip_buff = NULL;
  E.eclip->clip_type = CLIPBOARD_NONE;

  E.filename = NULL;
}

/* Frees up the memory allocated for the Editor object before shutting down */
void editorFreeEditor (void) {
  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
    E.erow[row_idx].row_len = 0;
    E.erow[row_idx].rndr_len = 0;
    free(E.erow[row_idx].row_str);
    free(E.erow[row_idx].rndr_str);
    free(E.erow[row_idx].rndr_cls);
  }
  E.num_row = 0;
  E.visual_anchor_x = 0; 
  E.visual_anchor_y = 0;

  // Freeing up the whole editor history buffer
  for (int buff_idx = 0; buff_idx < (int) E.ebuff->hbuff_len; buff_idx++) {
    for (int erow_idx = 0; erow_idx < E.ebuff->num_row_hbuff[buff_idx]; erow_idx++) {
      E.ebuff->erow_hbuff[buff_idx][erow_idx].row_len = 0;
      E.ebuff->erow_hbuff[buff_idx][erow_idx].rndr_len = 0;
      free(E.ebuff->erow_hbuff[buff_idx][erow_idx].row_str);
      free(E.ebuff->erow_hbuff[buff_idx][erow_idx].rndr_str);
      //free(E.ebuff->erow_buff[buff_idx][erow_idx].rndr_cls);
    }
    E.ebuff->crsr_x_hbuff[buff_idx] = 0;
    E.ebuff->crsr_y_hbuff[buff_idx] = 0;
    E.ebuff->num_row_hbuff[buff_idx] = 0;
    free(E.ebuff->erow_hbuff[buff_idx]);
  }
  E.ebuff->hbuff_len = 0;

  // Freeing the rest of the EBuffer object
  free(E.ebuff->erow_hbuff);
  free(E.ebuff->crsr_x_hbuff);
  free(E.ebuff->crsr_y_hbuff);
  free(E.ebuff->num_row_hbuff);

  E.estat.stat_len = 0;
  E.emsg.msg_len = 0;
  E.cmd.cmd_len = 0;

  // Freeing the editor clip object
  E.eclip->clip_type = 0;

  for (int clip_idx = 0; clip_idx < (int) E.eclip->num_row_eclip; clip_idx++) {
    E.eclip->eclip_buff[clip_idx].row_len = 0;
    E.eclip->eclip_buff[clip_idx].rndr_len = 0;

    free(E.eclip->eclip_buff[clip_idx].row_str);
    free(E.eclip->eclip_buff[clip_idx].rndr_str);
    //free(E.eclip->eclip_buff[clip_idx].rndr_cls);
  }

  E.eclip->num_row_eclip = 0;
  bufferFreeEClip();
  free(E.eclip);

  free(E.estat.stat_str);
  free(E.emsg.msg_str);
  free(E.cmd.cmd_str);
  free(E.srch.srch_str);
  free(E.srch.srch_match_x);
  free(E.srch.srch_match_y);
  free(E.filename);
}

/* Cleaning up the memory and recovering the default terminal. */
void editorExitEditor (void) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  disableRawMode();
  editorFreeEditor();
  exit(0);
}

