#include "common.h"
#include "getopt.h"

typedef unsigned char bool;
static bool opts[128];
static bool is_trmnl; /* is_stdout_terminal? */

void doTask(const char *fn);

int main(int argc, char *argv[]) {
  int opt, i;
  bool hsopt = 0;
  if(isatty(STDOUT_FILENO))
    is_trmnl = 1;
  else
    is_trmnl = 0;
  if(is_trmnl) {
    opts['C'] = 1;
    opts['q'] = 1;
  } else {
    opts['1'] = 1;
    opts['w'] = 1;
  }

  while((opt = getopt(argc, argv, "AacCdFfhiklnqRrSstuwx1")) != -1) {
    if(opt == 'w' || opt == 'q')
      opts['w'] = opts['q'] = 0;
    else if(opt == 'l' || opt == 'C' || opt == '1' || opt == 'n' || opt == 'x')
      opts['l'] = opts['C'] = opts['1'] = opts['n'] = opts['x'] = 0;
    else if(opt == 'c' || opt == 'u')
      opts['c'] = opts['u'] = 0;
    opts[opt] = 1;
    hsopt = 1;
  }
  printf("l=%d, C=%d, 1=%d, n=%d, x=%d\n", opts['l'], opts['C'], \
      opts['1'], opts['n'], opts['x']);
  if(hsopt) i = 2;
  else i = 1;
  i++;
  exit(0);
}

