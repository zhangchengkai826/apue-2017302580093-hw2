#include "common.h"
#include "getopt.h"

typedef unsigned char bool;
static bool opts[128];

void doTask(const char *fn);

int main(int argc, char *argv[]) {
  int opt, i;
  bool hsopt = 0;
  while((opt = getopt(argc, argv, "AacCdFfhiklnqRrSstuwx1")) != -1) {
    opts[opt] = 1;
    hsopt = 1;
  }
  if(hsopt) i = 2;
  else i = 1;
  for(; i < argc; i++) {
    doTask(argv[i]);
  }
  exit(0);
}

void doTask(const char *fn) {
  fputs(fn, stdout);
  fputs("\n", stdout);
}
