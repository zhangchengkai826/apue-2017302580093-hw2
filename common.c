#include "common.h"
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <fcntl.h>

#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

static long posix_version = 0;
static long xsi_version = 0;

#define PATH_MAX_GUESS 1024

char *path_alloc(size_t *sizep) {
  char *ptr;
  size_t size;

  if(posix_version == 0) posix_version = sysconf(_SC_VERSION);
  if(xsi_version == 0) xsi_version = sysconf(_SC_XOPEN_VERSION);
  if(pathmax == 0) {
    errno = 0;
    if((pathmax = pathconf("/", _PC_PATH_MAX)) < 0) {
      if(errno == 0) pathmax = PATH_MAX_GUESS; /* it's indeterminate */
      else err_sys("pathconf error for _PC_PATH_MAX");
    } else {
      pathmax++; /* add one since it's relative to root */
    }
  }

  /*
   * Before POSIX.1-2001, we aren't guaranteed that PATH_MAX includes
   * the terminating null byte.  Same goes for XPG3.
   */
  if((posix_version < 200112L) && (xsi_version < 4)) size = pathmax + 1;
  else size = pathmax;

  if((ptr = malloc(size)) == NULL) err_sys("malloc error for pathname");
  if(sizep != NULL) *sizep = size;
  return ptr;
}

#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

#define OPEN_MAX_GUESS 256

long open_max() {
  if(openmax == 0) {
    errno = 0;
    if((openmax = sysconf(_SC_OPEN_MAX)) < 0) {
      if(errno == 0) openmax = OPEN_MAX_GUESS;
      else err_sys("sysconf error for _SC_OPEN_MAX");
    }
  }
  return openmax;
}

void set_fl(int fd, int flags) { /* flags are file status flags to turn on */
  int val;
  if((val = fcntl(fd, F_GETFL, 0)) < 0)
    err_sys("fcntl F_GETFL error");
  val |= flags;
  if(fcntl(fd, F_SETFL, val) < 0)
    err_sys("fcntl F_SETFL error");
}

static void err_doit(int, int, const char *, va_list);

void err_sys(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  err_doit(1, errno, fmt, ap);
  va_end(ap);
  exit(1);
}

void err_msg(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  err_doit(0, 0, fmt, ap);
  va_end(ap);
}

void err_ret(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  err_doit(1, errno, fmt, ap);
  va_end(ap);
}

void err_quit(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  err_doit(0, 0, fmt, ap);
  va_end(ap);
  exit(1);
} 

void err_dump(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  err_doit(1, errno, fmt, ap);
  va_end(ap);
  abort();
  exit(1);
}

static void err_doit(int errnoflag, int error, const char *fmt, va_list ap) {
  char buf[MAXLINE];
  vsprintf(buf, fmt, ap);
  if(errnoflag)
    sprintf(buf+strlen(buf), ": %s", strerror(error));
  strcat(buf, "\n");
  fflush(stdout);
  fputs(buf, stderr);
  fflush(NULL);
}


