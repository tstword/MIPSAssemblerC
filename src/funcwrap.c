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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
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

/**
 * @function: get_tempdir
 * @purpose: Retrives the temporary directory.
 * @return A dynamically allocated string containing the temporary directory path. NULL if it fails.
 **/
char *get_tempdir() {
    char *tmpdir;
#ifdef _WIN32
    DWORD bufsize = GetTempPathA(0, NULL);
    if(bufsize == 0) {
        return strdup_wrap(".\\");
    }

    tmpdir = (char *)malloc(bufsize);
    if(tmpdir == NULL) {
        return NULL;
    }

    if(GetTempPathA(bufsize, tmpdir) == 0) {
        free(tmpdir);
        return strdup_wrap(".\\");
    }
#else
    const char *envarr[] = { "TMPDIR", "TEMP", "TMP" };  
    int envarr_size = sizeof(envarr) / sizeof(const char *);

    const char *dirarr[] = { "/tmp", "/var/tmp", "/usr/tmp" };
    int dirarr_size = sizeof(dirarr) / sizeof(char *);

    const char *envdir = NULL;

    const char *selectdir = NULL;

    /* Check environment variables first */
    for(int i = 0; selectdir == NULL && i < envarr_size; ++i) {
        if((envdir = getenv(envarr[i])) && access(envdir, R_OK | W_OK | X_OK) == 0) {
            selectdir = envdir;
        }
    }

    /* Check standard directories */
    for(int i = 0; selectdir == NULL && i < dirarr_size; ++i) {
        if(access(dirarr[i], R_OK | W_OK | X_OK) == 0) {
            selectdir = dirarr[i];
        }
    }

    /* Default to current directory */
    if(selectdir == NULL) {
        return strdup("./");
    }

    /* Dynamically allocate string and append directory seperator */
    int sdirlen = strlen(selectdir);

    tmpdir = (char *)malloc(sdirlen + 2);
    if(tmpdir == NULL) {
        return NULL;
    }

    strcpy(tmpdir, selectdir);
    tmpdir[sdirlen] = '/';
    tmpdir[sdirlen + 1] = 0;
#endif
    return tmpdir;
}

char *get_tempfile(const char *prefix, const char *suffix) {
    char *tempdir = get_tempdir();
    char *tempfile;
    
    if(tempdir == NULL) return NULL;
    if(prefix == NULL) prefix = "";
    if(suffix == NULL) suffix = "";
    
    int tmpdlen = strlen(tempdir);
    int preflen = strlen(prefix);
    int sufflen = strlen(suffix);

#ifdef _WIN32
    char *apibuffer = (char *)malloc(MAX_PATH);
	int apiblen, tmpflen;

    if(apibuffer == NULL) {
        free(tempdir);
        return NULL;
    }

    /* Make call to Windows API */
    if(GetTempFileNameA(tempdir, "", 0, apibuffer) == 0) {
        free(apibuffer);
        free(tempdir);
        return NULL;
    }

	apiblen = strlen(apibuffer);
	tmpflen = (apiblen - 4) + preflen + sufflen;

	if (tmpflen >= MAX_PATH) {
		free(apibuffer);
		free(tempdir);
		return NULL;
	}

	tempfile = (char*)malloc(tmpflen + 1);

	if(tempfile == NULL) {
		free(apibuffer);
		free(tempdir);
		return NULL;
	}

	memcpy((void *)tempfile, (void *)tempdir, tmpdlen);
	memcpy((void *)(tempfile + tmpdlen), (void *)prefix, preflen);
	memcpy((void *)(tempfile + tmpdlen + preflen), (void *)(apibuffer + tmpdlen), 4);
	memcpy((void *)(tempfile + tmpdlen + preflen + 4), (void *)suffix, sufflen + 1);
#else
    const char *template = "XXXXXX";

    /* Temporary file descriptor created from mkstemp */
    int tempfd;

    tempfile = (char *)malloc(tmpdlen + preflen + 6 + sufflen + 1);

    if(tempfile == NULL) {
        free(tempdir);
        return NULL;
    }

    memcpy((void *)tempfile, (void *)tempdir, tmpdlen);
    memcpy((void *)(tempfile + tmpdlen), (void *)prefix, preflen);
    memcpy((void *)(tempfile + tmpdlen + preflen), (void *)template, 6);
    memcpy((void *)(tempfile + tmpdlen + preflen + 6), (void *)suffix, sufflen + 1);

    /* Create unique temporary name using mkstemp */
    if((tempfd = mkstemps(tempfile, sufflen)) == -1) {
        free(tempdir);
        free(tempfile);
        return NULL;
    }

    /* Close file descriptor */
    if(close(tempfd) == -1) {
        perror("CRITICAL ERROR: Failed to close file descriptor during creation of temporary file: ");
        exit(EXIT_FAILURE);
    }
#endif
    /* Free used data */
    free(tempdir);

    return tempfile;
}