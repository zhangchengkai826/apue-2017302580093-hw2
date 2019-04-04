#include <dirent.h>
#include <sys/types.h>
#include "common.h"
#include "getopt.h"
#define MID_SIZE 8
#define LINE_MAX 1024

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
  if(f == NULL)
    return;
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

void getlf(char *buf, size_t n, const char *fn) { /* get long-format str */
  struct stat statbuf;
  if(lstat(fn, &statbuf) == -1)
    err_ret("ls: cannot access \'%s\'", fn);
  
}

static bool opts[128];
static bool is_trmnl; /* is_stdout_terminal? */
static struct Vector nrml, dirs; /* variable array for normal files & dirs */
static cmpfunc cf;
static enum SORT_ORDER odr;

void print_them(struct Vector *v, const char *blk_name) { 
  int i;
  if(blk_name != NULL)
    printf("%s:\n", blk_name);

  for(i = 0; i < (int)v->n; i++) {
    char fmt; /* file format */
    char *fn = v->d[i];
    char tag[2] = {0};
    struct stat statbuf;
    if(lstat(fn, &statbuf) < 0) 
      err_ret("ls: cannot access \'%s\'", fn);
    switch(statbuf.st_mode & __S_IFMT) {
      case __S_IFREG:
        fmt = '-';
        if(statbuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
          tag[0] = '*';
        break;
      case __S_IFDIR:
        fmt = 'd';
        tag[0] = '/';
        break;
      case __S_IFBLK:
        fmt = 'b';
        break;
      case __S_IFCHR:
        fmt = 'c';
        break;
      case __S_IFLNK:
        fmt = 'l';
        tag[0] = '@';
        break;
      case __S_IFSOCK:
        fmt = 's';
        tag[0] = '=';
        break;
      case __S_IFIFO:
        fmt = 'p';
        tag[0] = '|';
        break;
    }
    if(!opts['F'])
      tag[0] = '\0';
    if(opts['1'])
      printf("%s%s\n", fn, tag);
  }
  printf("\n");
}

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

  if(geteuid() == 0) 
    opts['A'] = 1;

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

  cf = cf_lex;
  odr = ASC;
  if(opts['f'])
    cf = NULL;
  
  if(opts['r']) {
    if(odr == ASC)
      odr = DESC;
    else if(odr == DESC)
      odr = ASC;
  }

  sort_vec(&nrml, cf, odr);
  sort_vec(&dirs, cf, odr);
  print_them(&nrml, NULL);

  for(i = 0; i < (int)dirs.n; i++) {
    DIR *dp;
    struct dirent *dirp;
    free_vec(&nrml);
    init_vec(&nrml, MID_SIZE);
    if((dp = opendir(dirs.d[i])) == NULL) {
      err_ret("ls: cannot access \'%s\'", dirs.d[i]);
      continue;
    }
    while((dirp = readdir(dp)) != NULL) {
      char *fn; /* directory entry name */
      fn = dirp->d_name;
      if(opts['a'] || (opts['A'] && strcmp(fn, ".") && strcmp(fn, "..")) || fn[0] != '.')
        appends_vec(&nrml, fn);
    }
    sort_vec(&nrml, cf, odr);
    print_them(&nrml, dirs.d[i]);
  }

  free_vec(&nrml);
  free_vec(&dirs);
  exit(0);
}

