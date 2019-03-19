#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#define MAXLINE 4096

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

char *path_alloc(size_t *sizep);

void err_msg(const char *, ...);
void err_sys(const char *, ...);
void err_quit(const char *, ...);
void err_ret(const char *, ...);
void err_dump(const char *, ...);

