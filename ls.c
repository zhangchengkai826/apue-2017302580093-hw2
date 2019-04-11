#include <dirent.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "common.h"
#include "getopt.h"
#include "math.h"
#include "pwd.h"
#include "grp.h"
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
static cmpfunc cf;
static enum SORT_ORDER odr;

long get_ino(const char *fn, char *szino) {
  int ino;
  struct stat statbuf;
  if(lstat(fn, &statbuf) < 0) 
    err_ret("ls: cannot access \'%s\'", fn);
  ino = statbuf.st_ino;
  if(szino)
    sprintf(szino, "%d", ino);
  return ino;
}

int get_mlen_szino(struct Vector *v) {
  int mlen = 0, i;
  for(i = 0; i < (int)v->n; i++) {
    char szino[LINE_MAX];
    get_ino(v->d[i], szino);
    if((int)strlen(szino) > mlen)
      mlen = strlen(szino);
  }
  return mlen;
}

void blkcnt2sz(int blkcnt, char *szblkcnt) {
  double val;
  char *blksize = getenv("BLOCKSIZE");
  if(blksize)
    val = (double)atoi(blksize) * blkcnt;
  else
    val = 512. * blkcnt; 
  
  if(opts['h']) {
    char units[] = "BKMGTPEZY";
    int i;
    for(i = 0; i < (int)strlen(units) && val/1024. >= 1.; i++, val /= 1024.);
    sprintf(szblkcnt, "%.1f%c", val, units[i]);
  } else if(opts['k']) {
    val /= 1024.;
    sprintf(szblkcnt, "%.1fK", val);
  } else {
    sprintf(szblkcnt, "%d", blkcnt);
  }
}

/* ret val is always blkcnt, szblkcnt will be affected by -hk */
int get_blkcnt(const char *fn, char *szblkcnt) {
  char *blksize;
  int blkcnt;
  struct stat statbuf;
  if(lstat(fn, &statbuf) < 0) 
    err_ret("ls: cannot access \'%s\'", fn);
  blkcnt = statbuf.st_blocks;
  blksize = getenv("BLOCKSIZE");
  if(blksize) {
    double ratio = 512.f / atoi(blksize);
    blkcnt = (int)ceil(blkcnt*ratio);
  }
  if(szblkcnt)
    blkcnt2sz(blkcnt, szblkcnt);
  return blkcnt;
}

/* get max strlen of szblkcnt */
int get_mlen_szblkcnt(struct Vector *v) {
  int mlen = 0, i;
  for(i = 0; i < (int)v->n; i++) {
    char szblkcnt[LINE_MAX];
    get_blkcnt(v->d[i], szblkcnt);
    if((int)strlen(szblkcnt) > mlen)
      mlen = strlen(szblkcnt);
  }
  return mlen;
}

int get_sum_blkcnt(struct Vector *v) {
  int sum = 0, i;
  for(i = 0; i < (int)v->n; i++)
    sum += get_blkcnt(v->d[i], NULL);
  return sum;
}

/* normalize file name */
char *nmfn(char *fn) {
  if(opts['q']) {
    char *p;
    p = fn;
    while(*p) {
      if(!isprint(*p))
        *p = '?';
      p++;
    }
  }
  return fn;
}

/* get max strlen in a string Vector */
int get_mlen_sv(const struct Vector *v) {
  int i, mlen;
  mlen = 0;
  for(i = 0; i < (int)v->n; i++) {
    int n;
    n = strlen(v->d[i]);
    if(n > mlen)
      mlen = n;
  }
  return mlen;
}

int get_tty_wid() {
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);
  return w.ws_col;
}

char *get_perm(const char *fn, char buf[]) {
  struct stat statbuf;
  mode_t m;
  if(lstat(fn, &statbuf) < 0) 
    err_ret("ls: cannot access \'%s\'", fn);
  m = statbuf.st_mode;
  
  if(m & S_IRUSR)
    buf[0] = 'r';
  else
    buf[0] = '-';
  
  if(m & S_IWUSR)
    buf[1] = 'w';
  else
    buf[1] = '-';
  
  if(m & S_IXUSR) {
    if(m & S_ISUID)
      buf[2] = 's';
    else
      buf[2] = 'x';
  } else {
    if(m & S_ISUID)
      buf[2] = 'S';
    else
      buf[2] = '-';
  }

  if(m & S_IRGRP)
    buf[3] = 'r';
  else
    buf[3] = '-';

  if(m & S_IWGRP)
    buf[4] = 'w';
  else
    buf[4] = '-';

  if(m & S_IXGRP) {
    if(m & S_ISGID) 
      buf[5] = 's';
    else
      buf[5] = 'x';
  } else {
    if(m & S_ISGID)
      buf[5] = 'S';
    else
      buf[5] = '-';
  }

  if(m & S_IROTH)
    buf[6] = 'r';
  else
    buf[6] = '-';
  
  if(m & S_IWOTH)
    buf[7] = 'w';
  else
    buf[7] = '-';

  if(m & S_IXOTH) {
    if(m & __S_ISVTX)
      buf[8] = 't';
    else
      buf[8] = 'x';
  } else {
    if(m & __S_ISVTX)
      buf[8] = 'T';
    else 
      buf[8] = '-';
  } 
  buf[9] = 0;
  return buf;
}

void print_them(struct Vector *v, const char *blk_name) { 
  int i;
  char oldwd[LINE_MAX]; /* old working dir */
  int mlen_szino, mlen_szblkcnt; 
  struct Vector mcv; /* used for multi-col print */

  getcwd(oldwd, sizeof(oldwd));
  chdir(blk_name);
  init_vec(&mcv, MID_SIZE);

  mlen_szino = get_mlen_szino(v);
  mlen_szblkcnt = get_mlen_szblkcnt(v);
  if(blk_name != NULL)
    printf("%s:\n", blk_name);
  if((opts['s'] && is_trmnl) || opts['l']) {
    char szblkcnt[LINE_MAX];
    blkcnt2sz(get_sum_blkcnt(v), szblkcnt);
    printf("total %s\n", szblkcnt);
  }

  for(i = 0; i < (int)v->n; i++) {
    char fmt; /* file format */
    char *fn = v->d[i];
    char tag[2] = {0};
    struct stat statbuf;
    char info[LINE_MAX];
    int n;
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
    n = 0; 
    if(opts['i'])
      n += sprintf(info + n, "%*ld ", mlen_szino, get_ino(fn, NULL)); 
    if(opts['s']) {
      char szblkcnt[LINE_MAX];
      get_blkcnt(fn, szblkcnt);
      n += sprintf(info + n, "%*s ", mlen_szblkcnt, szblkcnt);
    }
    if(!opts['F'])
      tag[0] = '\0';
    if(opts['l'] || opts['n']) {
      char perm[10];
      struct passwd *usr;
      struct group *grp;
      get_perm(fn, perm);
      n += sprintf(info + n, "%c", fmt);
      n += sprintf(info + n, "%s ", perm);
      n += sprintf(info + n, "%lu ", statbuf.st_nlink);
      usr = getpwuid(statbuf.st_uid);
      if(usr == NULL || opts['n'])
        n += sprintf(info + n, "%u ", statbuf.st_uid);
      else
        n += sprintf(info + n, "%s ", usr->pw_name);
      grp = getgrgid(statbuf.st_gid); 
      if(grp == NULL || opts['n'])
        n += sprintf(info + n, "%u ", statbuf.st_gid);
      else
        n += sprintf(info + n, "%s ", grp->gr_name);
      sprintf(info + n, "%s%s", fn, tag);
      printf("%s\n", nmfn(info));
    } else {
      sprintf(info + n, "%s%s", fn, tag);
      if(opts['1'])
        printf("%s\n", nmfn(info));
      else if(opts['C'] || opts['x'])
        appends_vec(&mcv, nmfn(info));
    }
  }

  if(mcv.n > 0 && (opts['C'] || opts['x'])) {
    int mlen, nc, nr; /* number of cols-rows */
    mlen = get_mlen_sv(&mcv);
    nc = get_tty_wid() / (mlen + 1);
    if(nc == 0) {
      for(i = 0; i < (int)mcv.n; i++) 
        printf("%s\n", (char*)mcv.d[i]);
    } else {
      if(mcv.n % nc == 0)
        nr = mcv.n / nc;
      else
        nr = mcv.n / nc + 1;
      for(i = 0; i < nc*nr; i++) {
        if(opts['x']) {
          if(i < (int)mcv.n)
            printf("%-*s ", mlen, (char*)mcv.d[i]);
        } else if(opts['C']) {
          int id;
          id = nr*(i%nc) + i/nc;
          if(id < (int)mcv.n)
            printf("%-*s ", mlen, (char*)mcv.d[id]);
          else
            printf("%-*s ", mlen, "");
        }
        if(i%nc+1 == nc)
          printf("\n");
      }
    }        
  }

  printf("\n");
  free_vec(&mcv);

  if(opts['R']) {
    struct Vector nrml, dirs;
    struct stat statbuf;
    init_vec(&nrml, MID_SIZE);
    init_vec(&dirs, MID_SIZE);

    for(i = 0; i < (int)v->n; i++) {
      char *fn;
      fn = v->d[i];
      if(lstat(fn, &statbuf) == -1) {
        err_ret("ls: cannot access \'%s\'", fn);
        continue;
      }
      if(S_ISDIR(statbuf.st_mode))
        appends_vec(&dirs, fn);
    }

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
  }

  chdir(oldwd);
}

int main(int argc, char *argv[]) {
  int opt, i;
  bool hsopt = 0;
  struct Vector nrml, dirs; 
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
    else if(opt == 'h' || opt == 'k')
      opts['h'] = opts['k'] = 0;
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

