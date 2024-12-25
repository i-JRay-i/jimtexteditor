#include "term.h"
#include "output.h"
#include "fileio.h"

#define CTRL_KEY(key) (key & 0x1f)

EConf E;

int main(int argc, char *argv[]) {
  initEditor();
  if (argc > 1) {
    openFile(argv[1]);
  }
  setMessage(HELP_MSG);
  while (1){
    refreshScreen();
    processKey();
  }
  return 0;
}
