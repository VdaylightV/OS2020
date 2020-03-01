#include <game.h>

// Operating system is a C program!
int main(const char *args) {
  _ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  game_bg();

  puts("Press any key to see its key code...\n");
  while (1) {
    _halt(0);
    print_key();
  }
  return 0;
}
