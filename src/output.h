#ifndef JIM_OUTPUT_H
#define JIM_OUTPUT_H

#include <unistd.h>
#include <string.h>
#include "term.h"

typedef struct output_buffer{
  char *str;
  int p_str;
  long size;
} OBuf;

void refreshScreen(void);

#endif
