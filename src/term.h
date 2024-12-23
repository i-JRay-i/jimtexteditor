#ifndef JIM_TERM_H
#define JIM_TERM_H

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define JIM_VERSION "0.0.1"
#define TAB_SIZE 8
#define PAGE_SIZE 64
#define HALF_PAGE_SIZE 32

typedef enum editor_mode {
  MODE_NORMAL,
  MODE_INSERT,
  MODE_COMMAND,
  MODE_VISUAL
} EMode;

typedef enum editor_key {
  MOVE_UP = 1000,
  MOVE_DOWN, 
  MOVE_LEFT,
  MOVE_RIGHT,
  MOVE_WORD_FORWARD,
  MOVE_WORD_BACKWARD,
  HALF_PAGE_UP,
  HALF_PAGE_DOWN,
  EOF_KEY,
  INIT_FILE_KEY,
  EOL_KEY,
  INIT_LINE_KEY,
  NORMAL_KEY,
  INSERT_KEY,
  COMMAND_KEY,
  ENTER_COMMAND_KEY,
  EXIT_KEY,
  ERASE_LEFT_KEY
} Key;

/* Editor row structure */
typedef struct editor_row {
  char *row_str; // Row string 
  int row_len; // Row string length
  char *rndr_str; // Rendered string
  int rndr_len; // Rendered string length
} ERow;

typedef struct editor_status {
  char stat_str[512];
  int stat_len;
} EStat;

typedef struct editor_message {
  char msg_str[512];
  int msg_len;
  int msg_time;
} EMsg;

typedef struct editor_command {
  char cmd_str[512];
  int cmd_len;
} CMD;

typedef struct editor_config { // Configuration variables for the editor
  int term_width, term_height;
  int crsr_x, crsr_y; // Cursor coordinates in the file
  int crsr_cmd_x, crsr_cmd_y; // Cursor coordinates in command mode
  int crsr_rndr_x, crsr_rndr_y; // Rendered cursor coordinates
  int row_off, col_off; // Editor row and column offest
  int num_row; // The total number of rows in the open file
  ERow *erow; // An array of ERow struct
  EMode emode; // Editor mode
  EStat estat; // Editor status bar object
  EMsg emsg; // Editor message bar object
  CMD cmd;
  struct termios term_conf_editor;
} EConf;

extern EConf E;

void die(const char*);
int readKey(void);
void printKey(void);
void processKey(void);
void editorScroll(void);
void initEditor(void);
void disableRawMode(void);
void appendStatusMessage(char *, int);
void appendMessage(char *, int);
int getWindowSize(int *, int *);
void insertChar(int);

#endif
