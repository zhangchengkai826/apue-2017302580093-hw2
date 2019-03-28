#include <sys/types.h>
#include "common.h"
#include "getopt.h"
#define MID_SIZE 8

typedef unsigned char bool;
struct Vector {
  void **d;
  size_t n;
  size_t cap; /* capacity */
};

void init_vec(struct Vector *v, size_t n) {
  v->d = malloc(n * sizeof(void*));
  v->n = 0;
  v->cap = n;
}

void append_vec(struct Vector *v, const void *item, size_t n) {
  void *item_dup;
  v->n++;
  if(v->n > v->cap) {
    v->cap *= 2;
    v->d = realloc(v->d, v->cap * sizeof(void*));
  }
  item_dup = malloc(n);
  memcpy(item_dup, item, n);
  v->d[v->n-1] = item_dup; 
}

void appends_vec(struct Vector *v, const char *str) {
  size_t n;
  n = strlen(str);
  append_vec(v, str, n+1);
}

void free_vec(struct Vector *v) {
  int i;
  for(i = 0; i < (int)v->n; i++) 
    free(v->d[i]);
  free(v->d);
}

/* test purpose only */
void print_vec(const struct Vector *v) {
  int i;
  fputs("[ ", stdout);
  for(i = 0; i < (int)v->n - 1; i++) 
    printf("%s, ", (char*)v->d[i]);
  printf("%s ]\n", (char*)v->d[i]);
}

/* returns a-b */
typedef int (*cmpfunc)(void *a, void *b);
enum SORT_ORDER {
  ASC,
  DESC
};

void swap_vec(struct Vector *v, int i, int j) {
  void *t;
  t = v->d[i];
  v->d[i] = v->d[j];
  v->d[j] = t;
}

void sort_vec(struct Vector *v, cmpfunc f, enum SORT_ORDER o) {
  bool chg;
  int i, j;
  for(chg = 0, i = 0, j = v->n; j-i > 1; chg = 0, i = 0, j--) {
    while(i + 1 < j) {
      switch(o) {
      case ASC:
        if(f(v->d[i], v->d[i+1]) > 0) {
          chg = 1;
          swap_vec(v, i, i+1);
        }
        break;
      case DESC:
        if(f(v->d[i], v->d[i+1]) < 0) {
          chg = 1;
          swap_vec(v, i, i+1);
        }
        break;
      }
      i++;
    }
    if(!chg)
      break;
  }
}

int cf_lex(void *a, void *b) {
  return strcmp(a, b);
}

static bool opts[128];
static bool is_trmnl; /* is_stdout_terminal? */
static struct Vector nrml, dirs; /* variable array for normal files & dirs */
static cmpfunc cf;

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
/*  printf("l=%d, C=%d, 1=%d, n=%d, x=%d\n", opts['l'], opts['C'], \
      opts['1'], opts['n'], opts['x']); */
  init_vec(&nrml, MID_SIZE);
  init_vec(&dirs, MID_SIZE);
  if(hsopt) i = 2;
  else i = 1;
  if(i == argc) {
    if(opts['d'])
      appends_vec(&nrml, ".");
    else
      appends_vec(&dirs, ".");
  }
  else {
    for(; i < argc; i++) {
      struct stat statbuf;
      if(lstat(argv[i], &statbuf) == -1) {
        err_ret("ls: cannot access \'%s\'", argv[i]);
        continue;
      }
      if(S_ISDIR(statbuf.st_mode) && !opts['d'])
        appends_vec(&dirs, argv[i]);
      else if(S_ISLNK(statbuf.st_mode) && !opts['d']) {
        if(stat(argv[i], &statbuf) == -1) {
          err_ret("ls: cannot access \'%s\'", argv[i]);
          continue;
        }
        if(S_ISDIR(statbuf.st_mode))
          appends_vec(&dirs, argv[i]);
        else
          appends_vec(&nrml, argv[i]);
      }
      else
        appends_vec(&nrml, argv[i]);
    }
  }

  print_vec(&nrml);
  print_vec(&dirs);
        
  cf = cf_lex;
  sort_vec(&nrml, cf, ASC);
  sort_vec(&dirs, cf, ASC);

  print_vec(&nrml);
  print_vec(&dirs);

  free_vec(&nrml);
  free_vec(&dirs);
  exit(0);
}

