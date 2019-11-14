#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "assembler.h"
#include "mipsfhdr.h"

void write_object_file(FILE *fp, struct assembler *assembler) {
    struct MIPS_file_header file_hdr;
    size_t nbytes;

    memset((void *)&file_hdr, 0, sizeof(file_hdr));

    /* Magic number */
    memcpy((void *)file_hdr.m_magic, "mips", 4);

    /* Endianness check */
    uint16_t endian = 0x0201; /* If little endian result is 1, big endian result is 2 */
    file_hdr.m_endianness = *((uint8_t *)&endian);

    /* Version */
    file_hdr.m_version = 0x1;

    /* Section header count */
    file_hdr.m_shnum = 0;
    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        if(assembler->segment_memory_offset[segment] > 0) {
            ++file_hdr.m_shnum;
        }
    }

    /* Write header to object file */
    nbytes = fwrite((void *)&file_hdr, 0x1, sizeof(file_hdr), fp);
    if(nbytes != sizeof(file_hdr)) {
        fprintf(stderr, "Object Write Error: Failed to write file header: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    offset_t file_offset = sizeof(file_hdr);

    /* Write segment headers and their corresponding data */
    struct MIPS_sect_header section_hdr;

    /* Zero memory */
    memset((void *)&section_hdr, 0, sizeof(section_hdr));

    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        if(assembler->segment_memory_offset[segment] > 0) {
            section_hdr.sh_segment = segment;
            section_hdr.sh_offset = file_offset;
            section_hdr.sh_size = assembler->segment_memory_offset[segment];

            nbytes = fwrite((void *)&section_hdr, 0x1, sizeof(section_hdr), fp);
            if(nbytes != sizeof(section_hdr)) {
                fprintf(stderr, "Object Write Error: Failed to write section header header: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            nbytes = fwrite(assembler->segment_memory[segment], 0x1, assembler->segment_memory_offset[segment], fp);
            if(nbytes != assembler->segment_memory_offset[segment]) {
                fprintf(stderr, "Object Write Error: Failed to write memory to file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            file_offset += sizeof(section_hdr) + assembler->segment_memory_offset[segment];
        }
    }
}

void display_help_msg(char *program) {
    printf("Usage: %s [-a] [-h] [-o output] file...\n", program);
    printf("A MIPS assembler written in C\n\n");
    printf("The following options may be used:\n");
    printf("  %-20s Only assembles program, does not create object file\n", "-a");
    printf("  %-20s Displays this message\n", "-h");
    printf("  %-20s Stores object code in <output>\n\n", "-o <output>");
    printf("Refer to the repository at <https://github.com/tstword/MIPSAssemblerC>\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    FILE *output_fp = NULL;
    const char *output_file = "a.obj";
    int assemble_only = 0, display_help = 0;
    char **input_file;
    size_t file_count;
    
#ifndef _WIN32
    int opt;
    while((opt = getopt(argc, argv, "aho:")) != -1) {
        switch(opt) {
            case 'a':
                assemble_only = 1;
                break;
            case 'h':
                display_help = 1;
                break;
            case 'o':
                output_file = optarg;
                break;
            default: /* '?' */
                fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    input_file = argv + optind;
    file_count = argc - optind;
#else
    /* Grr we have to parse manually, sloppy but will get the job done */
    input_file = (char **)malloc(sizeof(char *) * (argc - 1));
    file_count = 0;

    int skip_index = 0;

    for(int i = 1; i < argc; ++i) {
        if(*(argv[i]) == '-') {
            while(*(++argv[i])) {
                switch(*(argv[i])) {
                    case 'a':
                        assemble_only = 1;
                        break;
                    case 'h':
                        display_help = 1;
                        break;
                    case 'o':
                        if(i + 1 == argc || *(argv[i + 1]) == '-') {
                            fprintf(stderr, "%s: option requires an argument -- 'o'\n", argv[0]);
                            return EXIT_FAILURE;
                        }
                        output_file = argv[i + 1];
                        skip_index = 1;
                        break;
                    default: /* ? */
                        fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], *(argv[i]));
                        fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
                        return EXIT_FAILURE;
                }
            }
            if(skip_index) {
                skip_index = 0;
                ++i;
            }
        }
        else {
            input_file[file_count++] = argv[i];
        }
    }
#endif

    if(display_help) display_help_msg(argv[0]);

    if(file_count == 0) {
        fprintf(stderr, "%s: Error: no input files\n", argv[0]);
        fprintf(stderr, "\nSee '%s -h' for more information\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct assembler *assembler = create_assembler();
    astatus_t status = execute_assembler(assembler, (const char **)input_file, file_count);
#ifdef _WIN32
    free(input_file)
#endif
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
        fclose(output_fp);
    }
    destroy_assembler(&assembler);

    return EXIT_SUCCESS;
}
