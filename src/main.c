#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "assembler.h"

int main(int argc, char *argv[]) {
    FILE *output_fp = NULL;
    const char *output_file = "a.obj";
    int assemble_only = 0;
    
    int opt;
    while((opt = getopt(argc, argv, "aho:")) != -1) {
        switch(opt) {
            case 'a':
                assemble_only = 1;
                break;
            case 'h':
                printf("Usage: %s [-h] [-a] [-o OUTPUT] FILE\n", argv[0]);
                printf("A MIPS assembler written in C\n\n");
                printf("The following options may be used:\n");
                printf("  -h\t\tDisplays this message\n");
                printf("  -a\t\tOnly assembles program, does not create object file\n");
                printf("  -o\t\tFile to store object code in\n\n");
                printf("Refer to the repository at <https://github.com/tstword/NewMIPSAssembler>\n");
                return EXIT_SUCCESS;
            case 'o':
                output_file = strdup(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if(optind >= argc) {
        fprintf(stderr, "%s: Error: no input files\n", argv[0]);
        fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct assembler *assembler = create_assembler();
    astatus_t status = execute_assembler(assembler, (const char **)(argv + optind), argc - optind);
    if(status != ASSEMBLER_STATUS_OK) {
        fprintf(stderr, "\nFailed to assemble program\n");
        return EXIT_FAILURE;
    }
    else if(assemble_only) {
        if((output_fp = fopen(output_file, "wb+")) == NULL) {
            fprintf(stderr, "Failed to open output file '%s' : %s\n", output_file, strerror(errno));
            return EXIT_FAILURE;
        }
        fwrite(assembler->segment_memory[SEGMENT_TEXT], 1, assembler->segment_memory_offset[SEGMENT_TEXT], output_fp);
        fwrite(assembler->segment_memory[SEGMENT_DATA], 1, assembler->segment_memory_offset[SEGMENT_DATA], output_fp);
        fclose(output_fp);
    }
    destroy_assembler(&assembler);

    return EXIT_SUCCESS;
}