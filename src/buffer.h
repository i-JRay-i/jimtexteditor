#ifndef JIM_BUFFER_H
#define JIM_BUFFER_H

#include "term.h"

extern void bufferSaveEditorState (void);
extern void bufferEditorUndo (void);
extern void bufferEditorRedo (void);
extern void bufferFreeEClip (void);
extern void bufferCopyToEClip (int, int);
extern void bufferPasteEClip (void);

#endif // JIM_BUFFER_H
