/**
 * @file: funcwrap.c
 *
 * @purpose: The Microsoft MSVC compiler complains quite a bit about the functions
 * declared in this file, opting to use "safe" versions of these functions. 
 * These wrapper functions are utilized to keep the code portable without having
 * to disable said warnings through compile flags.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (12/21/2019)
 **/

#include "funcwrap.h"

#include <string.h>
#ifdef _WIN32
#include <stdlib.h>
#endif

/**
 * @function: fopen_wrap
 * @purpose: Wrapper function for fopen designed to be portable with the MSVC compiler
 * @param filename -> The name of the file to open
 * @param mode     -> The mode in which to open the file with
 * @return The address of the FILE structure that was created with fopen or fopen_s
 **/
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

/**
 * @function: strdup_wrap
 * @purpose: Wrapper function for strdup designed to be portable with the MSVC compiler
 * @param src -> The string to duplicate
 * @return The address of the duplicated string
 **/
char *strdup_wrap(const char *src) {
#ifdef _WIN32
    size_t bufsize = strlen(src) + 1;
    char *buffer = (char *)malloc(bufsize);
    if(buffer == NULL) return NULL;
    memcpy((void *)buffer, (const void *)src, bufsize);
    return buffer;
#else
    return strdup(src);
#endif
}