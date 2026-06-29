#ifndef JIM_INPUT_H
#define JIM_INPUT_H

#include "term.h"

void erowAppend(int, char *, size_t);
void erowAppendString(ERow *, char *, size_t);
void erowInsertChar(ERow *, int, int);
void erowInsertString(ERow *, int, char *, size_t);
void erowDeleteChar(ERow *, int);
void erowDeleteRow(int);

#endif // JIM_INPUT_H
