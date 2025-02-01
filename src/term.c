#include "term.h"

#define CTRL_KEY(key) (key & 0x1f)

struct termios term_conf_def;

void die(const char* err_msg) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(err_msg);
  disableRawMode();
  freeEditor();
  exit(1);
}

int getCursorPosition(int *rows, int *cols) { // Fallback mechanism to read the cursor crsr_position
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

int getWindowSize(int *rows, int *cols) {
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

void moveCursor(int key) {
  ERow *erow = (E.crsr_y < E.num_row) ? &E.erow[E.crsr_y] : NULL;
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
      if (E.crsr_y < E.num_row)
        E.crsr_y++;
      break;
    case MOVE_WORD_FORWARD:
      while(erow) {
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
      break;
    case MOVE_WORD_FORWARD_END:
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
      break;
    case MOVE_WORD_BACKWARD:
      if (!erow) {
        E.crsr_y--;
        erow = (E.crsr_y >= E.num_row) ? NULL : &E.erow[E.crsr_y];
        E.crsr_x = E.erow[E.crsr_y].row_len - 1;
        break;
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
      break;
    case MOVE_TOKEN_FORWARD: 
      while (erow) {
        if (erow->row_str[E.crsr_x] == ' ') {
          E.crsr_x++;
          if (erow->row_str[E.crsr_x] != ' ')
            break;
        } else if (erow->row_str[E.crsr_x]  == '\0'){
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
      break;
    case MOVE_TOKEN_FORWARD_END:
      E.crsr_x++;
      while (erow) {
        if (erow->row_str[E.crsr_x] == ' ') {
          E.crsr_x++;
        } else if (erow->row_str[E.crsr_x]  == '\0') {
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
      break;
    case MOVE_TOKEN_BACKWARD:
      if (!erow) {
        E.crsr_y--;
        erow = (E.crsr_y >= E.num_row) ? NULL : &E.erow[E.crsr_y];
        E.crsr_x = E.erow[E.crsr_y].row_len - 1;
        break;
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
      break;
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

void moveCursorCommand(int key) {
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

int readKey(void) {
  int key = 0;
  if ((read(STDIN_FILENO, &key, 1)) == -1)
    die("read");
  return key;
}

/* Used for debugging */
void printKey(void) {
  int key = readKey();
  printf("%d ('%c')\r\n", key, key);
  write(STDOUT_FILENO, &key, 1);
}

int mapKeyNormal(int key) {
  switch (key){
    case CTRL_KEY('q'): return EXIT_KEY;
    case 'h': return MOVE_LEFT;
    case 'j': return MOVE_DOWN;
    case 'k': return MOVE_UP;
    case 'l': return MOVE_RIGHT;
    case 'i': return INSERT_KEY;
    case 'a': return INSERT_NEXT_KEY;
    case 'o': return INSERT_LINE_NEXT;
    case 'O': return INSERT_LINE_PREV;
    case ':': return COMMAND_KEY;
    case 'W': return MOVE_WORD_FORWARD;
    case 'E': return MOVE_WORD_FORWARD_END;
    case 'B': return MOVE_WORD_BACKWARD;
    case 'w': return MOVE_TOKEN_FORWARD;
    case 'e': return MOVE_TOKEN_FORWARD_END;
    case 'b': return MOVE_TOKEN_BACKWARD;
    case CTRL_KEY('u'): return HALF_PAGE_UP;
    case CTRL_KEY('d'): return HALF_PAGE_DOWN;
    case 'G': return EOF_KEY;
    case '$': return EOL_KEY;
    case '0': return INIT_LINE_KEY;
    case '/': return SEARCH_KEY;
    case 'n': return SEARCH_FORWARD;
    case 'N': return SEARCH_BACKWARD;
    default: return key;
  }
}

void processNormal(int key) {
   switch (key) {
    case EXIT_KEY:
      exitEditor();
      break;
    case INSERT_KEY:
      E.emode = MODE_INSERT;
      write(STDOUT_FILENO, "\x1b[5 q", 5);
      fflush(stdout);
      break;
    case INSERT_NEXT_KEY:
      if (E.erow && E.crsr_x < E.erow[E.crsr_y].row_len)
        E.crsr_x++;
      E.emode = MODE_INSERT;
      write(STDOUT_FILENO, "\x1b[5 q", 5);
      fflush(stdout);
      break;
    case INSERT_LINE_PREV:
      break;
    case INSERT_LINE_NEXT:
      break;
    case COMMAND_KEY:
      E.emode = MODE_COMMAND;
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
        if (read(STDIN_FILENO, &escseq[0], 1) != 1) return NORMAL_KEY;
        if (read(STDIN_FILENO, &escseq[1], 1) != 1) return NORMAL_KEY;

        if (escseq[0] == '[') {
          if (read(STDIN_FILENO, &escseq[2], 1) != 1) return NORMAL_KEY;
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
        return NORMAL_KEY;
      }
    default: return key;
  }
}

void processInsert (int key) {
   switch (key) {
    case EXIT_KEY:
      exitEditor();
      break;
    case NORMAL_KEY:
      E.emode = MODE_NORMAL;
      write(STDOUT_FILENO, "\x1b[2 q", 5);
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

int mapKeyCommand(int key) {
  switch (key){
    case '\x1b': 
      {
      char esc_seq[6];
        if (read(STDIN_FILENO, &esc_seq[0], 1) != 1)
          return NORMAL_KEY;
        if (read(STDIN_FILENO, &esc_seq[1], 1) != 1)
          return NORMAL_KEY;
        if (esc_seq[0] == '[') {
          switch (esc_seq[1]) {
            case 'A': return MOVE_UP;
            case 'B': return MOVE_DOWN;
            case 'C': return MOVE_RIGHT;
            case 'D': return MOVE_LEFT;
          }
        }
      }
      return NORMAL_KEY;
    case '\r': return ENTER_COMMAND_KEY;
    case 127: return ERASE_LEFT_KEY;
    default: return key;
  }
}

void enterCommand(void) {
  if (!strcmp(E.cmd.cmd_str, "q")) {
    if ((E.dirt_flag_pos || E.dirt_flag_neg)) {
      setMessage("Warning: File has unsaved changes.");
      return;
    }
    exitEditor();
  } else if (!strcmp(E.cmd.cmd_str, "w")) {
    saveFile();
  } else if (!strcmp(E.cmd.cmd_str, "wq")) {
    saveFile();
    exitEditor();
  } else if (!strcmp(E.cmd.cmd_str, "q!")) {
    exitEditor();
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

void processCommand (int key) {
  switch (key) {
    case '\0':
      break;
    case NORMAL_KEY:
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

void searchQuery(void) {
  E.crsr_x = E.srch.srch_match_x[E.srch.srch_match_idx];
  E.crsr_y = E.srch.srch_match_y[E.srch.srch_match_idx];
}

void processSearch (int key) {
  switch (key) {
    case '\0':
      break;
    case NORMAL_KEY:
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

void searchPrompt(void) {
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

    if ((key == ENTER_COMMAND_KEY) || (key == NORMAL_KEY)) {
      break;
    }

    appendMessageString("/", 1);
    appendMessageString(E.srch.srch_str, E.srch.srch_len);
    refreshScreen();
  }
}

void commandPrompt(void) {
  while (1) {
    int key = readKey();
    key = mapKeyCommand(key);
    processCommand(key);
    if ((key == ENTER_COMMAND_KEY) || (key == NORMAL_KEY))
      break;

    appendMessageString(":", 1);
    appendMessageString(E.cmd.cmd_str, E.cmd.cmd_len);
    refreshScreen();
  }
}

void processKey(void) {
  int key = readKey();
  if (E.emode == MODE_NORMAL) {
    key = mapKeyNormal(key);
    processNormal(key);
  } else if (E.emode == MODE_INSERT) {
    key = mapKeyInsert(key);
    processInsert(key);
  } else if (E.emode == MODE_COMMAND) {
    commandPrompt();
  }
}

void appendStatusString(char *str, unsigned int str_len) {
  if (E.estat.stat_len + str_len > E.estat.stat_size) {
    E.estat.stat_size += 512;
    E.estat.stat_str = realloc(E.estat.stat_str, E.estat.stat_size);
  }

  memcpy(&E.estat.stat_str[E.estat.stat_len], str, str_len);
  E.estat.stat_str[E.estat.stat_len+str_len] = '\0';
  E.estat.stat_len = strlen(E.estat.stat_str);
}

void appendMessageString(char *str, unsigned int str_len) {
  if (E.emsg.msg_len + str_len > E.emsg.msg_size) {
    E.emsg.msg_size += 512;
    E.emsg.msg_str = realloc(E.emsg.msg_str, E.emsg.msg_size);
  }

  memcpy(&E.emsg.msg_str[E.emsg.msg_len], str, str_len);
  E.emsg.msg_str[E.emsg.msg_len+str_len] = '\0';
  E.emsg.msg_len = strlen(E.emsg.msg_str);
}

void setMessage(const char *fmt, ...) {
  char msg_buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg_buf, 256, fmt, ap);
  va_end(ap);
  appendMessageString(msg_buf, strlen(msg_buf));
  E.emsg.msg_time = time(NULL);
}

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_conf_def) == -1)
    die("tcsetattr");
}

void enableRawMode(void) {
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

void initEditor(void) {
  enableRawMode();
  getWindowSize(&E.term_height, &E.term_width);

  E.crsr_x = 0; E.crsr_y = 0;
  E.crsr_cmd_x = 0; E.crsr_cmd_y = 0;
  E.crsr_rndr_x = 0; E.crsr_rndr_y = 0;
  E.row_off = 0; E.col_off = 0;
  E.dirt_flag_pos = 0; E.dirt_flag_neg = 0;
  E.emode = MODE_NORMAL;

  // Initially there is no rows to draw
  E.erow = NULL;
  E.num_row = 0;

  E.estat.stat_size = 512;
  E.estat.stat_str = malloc(E.estat.stat_size);
  E.estat.stat_str[0] = '\0';
  E.estat.stat_len = 0;

  E.emsg.msg_size = 512;
  E.emsg.msg_str = malloc(E.emsg.msg_size);
  E.emsg.msg_str[0] = '\0';
  E.emsg.msg_len = 0;
  E.emsg.msg_time = 0;

  E.cmd.cmd_size = 512;
  E.cmd.cmd_str = malloc(E.cmd.cmd_size);
  E.cmd.cmd_str[0] = '\0';
  E.cmd.cmd_len = 0;

  E.srch.srch_size = 512;
  E.srch.srch_str = malloc(E.srch.srch_size);
  E.srch.srch_str[0] = '\0';
  E.srch.srch_len = 0;
  E.srch.srch_match_num = 0;
  E.srch.srch_match_idx = 0;
  E.srch.srch_match_x = NULL;
  E.srch.srch_match_y = NULL;

  E.filename = NULL;
}

void freeEditor(void) {
  for (int row_idx = 0; row_idx < E.num_row; row_idx++) {
    free(E.erow[row_idx].row_str);
    free(E.erow[row_idx].rndr_str);
  }

  free(E.estat.stat_str);
  free(E.emsg.msg_str);
  free(E.cmd.cmd_str);
  free(E.srch.srch_str);
  free(E.srch.srch_match_x);
  free(E.srch.srch_match_y);
}

void exitEditor(void) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  disableRawMode();
  freeEditor();
  exit(0);
}

