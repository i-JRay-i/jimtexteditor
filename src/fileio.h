#ifndef JIM_FILEIO_H
#define JIM_FILEIO_H

#include <sys/types.h>
#include <fcntl.h>
#include "term.h"
#include "output.h"

void openFile(char *);
void erowAppend(char *, size_t);
void erowInsertChar(ERow *, int, int);

#endif 
