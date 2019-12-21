#include "funcwrap.h"

#include <string.h>
#ifdef _WIN32
#include <stdlib.h>
#endif

FILE *fopen_wrap(const char *filename, const char *mode) {
#ifdef _WIN32
    FILE *fp;
    errno_t err = fopen_s(&fp, filename, mode);
    if(fp == NULL) errno = err;
    return fp;
#else
    return fopen(filename, mode);
#endif
}

char *strdup_wrap(const char *src) {
#ifdef _WIN32
    char *buffer = (char *)malloc(strlen(src) + 1);
    strcpy(buffer, src);
    return buffer;
#else
    return strdup(src);
#endif
}