#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "assembler.h"
#include "objhdr.h"

void write_object_file(FILE *fp, struct assembler *assembler) {
    struct mips_obj_fhdr fhdr = { {0}, {'m', 'i', 'p', 's'} };
    size_t nbytes;

    /* Endianness check */
    uint16_t endian = 0x0201; /* If little endian result is 1, big endian result is 2 */
    fhdr.m_endianness = *((uint8_t *)&endian);

    /* Version */
    fhdr.m_version = 0x1;

    /* Section header count */
    fhdr.m_shnum = 0;
    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        if(assembler->segment_memory_offset[segment] > 0) {
            ++fhdr.m_shnum;
        }
    }

    /* Write header to object file */
    nbytes = fwrite((void *)&fhdr, 0x1, sizeof(fhdr), fp);
    if(nbytes != sizeof(fhdr)) {
        fprintf(stderr, "Object Write Error: Failed to write file header: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    offset_t file_offset = sizeof(fhdr);

    /* Write segment headers and their corresponding data */
    struct mips_obj_shdr shdr = { {0, 0, 0 } };
    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        if(assembler->segment_memory_offset[segment] > 0) {
            shdr.sh_segment = segment;
            shdr.sh_offset = file_offset;
            shdr.sh_size = assembler->segment_memory_offset[segment];

            nbytes = fwrite((void *)&shdr, 0x1, sizeof(shdr), fp);
            if(nbytes != sizeof(shdr)) {
                fprintf(stderr, "Object Write Error: Failed to write section header header: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            nbytes = fwrite(assembler->segment_memory[segment], 0x1, assembler->segment_memory_offset[segment], fp);
            if(nbytes != assembler->segment_memory_offset[segment]) {
                fprintf(stderr, "Object Write Error: Failed to write memory to file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            file_offset += sizeof(shdr) + assembler->segment_memory_offset[segment];
        }
    }
}

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
                output_file = optarg;
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
    else if(!assemble_only) {
        if((output_fp = fopen(output_file, "wb+")) == NULL) {
            fprintf(stderr, "Failed to open output file '%s' : %s\n", output_file, strerror(errno));
            return EXIT_FAILURE;
        }
        write_object_file(output_fp, assembler);
        //fwrite(assembler->segment_memory[SEGMENT_TEXT], 1, assembler->segment_memory_offset[SEGMENT_TEXT], output_fp);
        //fwrite(assembler->segment_memory[SEGMENT_DATA], 1, assembler->segment_memory_offset[SEGMENT_DATA], output_fp);
        fclose(output_fp);
    }
    destroy_assembler(&assembler);

    return EXIT_SUCCESS;
}
