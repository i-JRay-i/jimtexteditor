#ifndef JIM_FILEIO_H
#define JIM_FILEIO_H

#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include "term.h"
#include "output.h"

void openFile(char *);
void erowAppend(char *, size_t);
void erowAppendString(ERow *, char *, size_t);
void erowInsertChar(ERow *, int, int);
void erowDeleteChar(ERow *, int);
void erowDelete(int);

#endif 
