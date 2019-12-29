/**
 * @file: funcwrap.h
 *
 * @purpose: The Microsoft MSVC compiler complains quite a bit about the functions
 * declared in this header, opting to use "safe" versions of these functions. 
 * These wrapper functions are utilized to keep the code portable without having
 * to disable said warnings through compile flags. 
 *
 * @author: Bryan Rocha
 * @version: 1.0 (12/21/2019)
 **/

#ifndef FUNCWRAP_H
#define FUNCWRAP_H

#include <stdio.h>

FILE *fopen_wrap(const char *, const char *);
char *strdup_wrap(const char *);
char *get_tempdir();

#endif