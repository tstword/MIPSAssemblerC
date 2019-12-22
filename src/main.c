/**
 * @file: main.c
 *
 * @purpose: Entry point for the MIPS assembler. Defines the necessary functions
 * for creating object files and dumping segments. 
 *
 * The following options may be used:
 *  -a                   Only assembles program, does not create object code file
 *                       * Note: This does not disable segment dumps
 *  -d <output>          Stores data segment in <output>
 *  -h                   Displays this message
 *  -t <output>          Stores text segment in <output>
 *  -o <output>          Stores object code in <output>
 *                       * Note: If this option is not specified, <output> defaults to a.obj
 *
 * @author: Bryan Rocha
 * @version: 1.0 (11/11/2019)
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "assembler.h"
#include "mipsfhdr.h"

void display_help_msg(char *program) {
    printf("Usage: %s [-a] [-h] [-t output] [-d output] [-o output] file...\n", program);
    printf("A MIPS assembler written in C\n\n");
    printf("The following options may be used:\n");
    printf("  %-20s Only assembles program, does not create object code file\n", "-a");
    printf("  %-20s * Note: This does not disable segment dumps\n", "");
    printf("  %-20s Stores data segment in <output>\n", "-d <output>");
    printf("  %-20s Displays this message\n", "-h");
    printf("  %-20s Stores text segment in <output>\n", "-t <output>");
    printf("  %-20s Stores object code in <output>\n", "-o <output>");
    printf("  %-20s * Note: If this option is not specified, <output> defaults to a.obj\n\n", "");
    printf("Refer to the repository at <https://github.com/tstword/MIPSAssemblerC>\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    const char *output_file = "a.obj";
    const char *text_file = NULL;
    const char *data_file = NULL;
    int assemble_only = 0, display_help = 0;
    
    const char **input_array;
    size_t input_count;
    
#ifndef _WIN32
    int opt;
    while((opt = getopt(argc, argv, "aho:t:d:")) != -1) {
        switch(opt) {
            case 'a':
                assemble_only = 1;
                break;
            case 'h':
                display_help = 1;
                break;
            case 't':
                text_file = optarg;
                break;
            case 'd':
                data_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            default: /* '?' */
                fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    input_array = (const char **)(argv + optind);
    input_count = argc - optind;
#else
    /* Grr we have to parse manually, sloppy but will get the job done */
    /* We could use this parsing algorithm for Linux / Mac, but I trust getopt more */
    /* Windows users will have to report errors (if any) */

    input_array = (const char **)malloc(sizeof(const char *) * (argc - 1));
    
    if(input_array == NULL) { 
        perror("CRITICAL ERROR: Failed to allocate memory input_array: ");
        exit(EXIT_FAILURE);
    }
    
    input_count = 0;

    int skip_index = 0, ch;

    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            skip_index = 0;
            while((ch = *(++argv[i])) != '\0') {
                switch(ch) {
                    case 'a':
                        assemble_only = 1;
                        break;
                    case 'h':
                        display_help = 1;
                        break;
                    case 'o':
                        if(i + 1 == argc || argv[i + 1][0] == '-') {
                            fprintf(stderr, "%s: option requires an argument -- 'o'\n", argv[0]);
                            return EXIT_FAILURE;
                        }
                        output_file = argv[i + 1];
                        skip_index = 1;
                        break;
                    case 't':
                        if(i + 1 == argc || argv[i + 1][0] == '-') {
                            fprintf(stderr, "%s: option requires an argument -- 't'\n", argv[0]);
                            return EXIT_FAILURE;
                        }
                        text_file = argv[i + 1];
                        skip_index = 1;
                        break;
                    case 'd':
                        if(i + 1 == argc || argv[i + 1][0] == '-') {
                            fprintf(stderr, "%s: option requires an argument -- 'd'\n", argv[0]);
                            return EXIT_FAILURE;
                        }
                        data_file = argv[i + 1];
                        skip_index = 1;
                        break;
                    default: /* ? */
                        fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], ch);
                        fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
                        return EXIT_FAILURE;
                }
            }
            if(skip_index) ++i;
        }
        else input_array[input_count++] = (const char *)argv[i];
    }
#endif

    if(display_help) display_help_msg(argv[0]);

    if(input_count == 0) {
        fprintf(stderr, "%s: Error: no input files\n", argv[0]);
        fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct assembler *assembler = create_assembler();
    astatus_t status = execute_assembler(assembler, input_array, input_count);

#ifdef _WIN32
    free(input_array);   /* I'm annoyed that I have to do this but oh well, nothing to do right now. */
#endif
    if(status != ASSEMBLER_STATUS_OK) {
        fprintf(stderr, "\nFailed to assemble program\n");
        destroy_assembler(&assembler);
        return EXIT_FAILURE;
    }
    else if(!assemble_only) {
        write_object_file(assembler, output_file);
    }

    /* Dump segments to file if specified */
    if(text_file != NULL) dump_segment(assembler, SEGMENT_TEXT, text_file);
    if(data_file != NULL) dump_segment(assembler, SEGMENT_DATA, data_file);

    destroy_assembler(&assembler);

    return EXIT_SUCCESS;
}
