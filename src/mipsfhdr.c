/**
 * @file: mipsfhdr.c
 *
 * @purpose: Defines the necessary functions creating object files and dumping segments
 * created by the assembler.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (11/4/2019)
 **/

#include "mipsfhdr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "funcwrap.h"

/**
 * @function: write_object_file
 * @purpose: Creates an object file based on the assembler provided and stores the binary data
 * into the specified file.
 * @param assembler -> The address of the assembler structure
 * @param file      -> The name of the file to write the data to
 **/
void write_object_file(struct assembler *assembler, const char *file) {
    struct MIPS_file_header file_hdr;
    size_t nbytes;
    FILE *fp;

    if((fp = fopen_wrap(file, "wb+")) == NULL) {
        fprintf(stderr, "Failed to open output file '%s: Error: ", file);
        perror(NULL);
        destroy_assembler(&assembler);
        exit(EXIT_FAILURE);
    }

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
        perror("Object Write Error: Failed to write file header: ");
        destroy_assembler(&assembler);
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
                perror("Object Write Error: Failed to write section header header: ");
                destroy_assembler(&assembler);
                exit(EXIT_FAILURE);
            }

            nbytes = fwrite(assembler->segment_memory[segment], 0x1, assembler->segment_memory_offset[segment], fp);
            if(nbytes != assembler->segment_memory_offset[segment]) {
                perror("Object Write Error: Failed to write memory to file: ");
                destroy_assembler(&assembler);
                exit(EXIT_FAILURE);
            }

            file_offset += sizeof(section_hdr) + assembler->segment_memory_offset[segment];
        }
    }

    fclose(fp);
}

/**
 * @function: dump_segment
 * @purpose: Stores the corresponding segment data into the specified file
 * @param assembler -> The address of the assembler structure
 * @param segment   -> The segment to dump
 * @param file      -> The name of the file to write the data to
 **/
void dump_segment(struct assembler *assembler, segment_t segment, const char *file) {
    FILE *fp;
    if((fp = fopen_wrap(file, "wb+")) == NULL) {
        fprintf(stderr, "Failed to open output file '%s': Error: ", file);
        perror(NULL);
        destroy_assembler(&assembler);
        exit(EXIT_FAILURE);
    }

    size_t nbytes = fwrite(assembler->segment_memory[segment], 0x1, assembler->segment_memory_offset[segment], fp);
    if(nbytes != assembler->segment_memory_offset[segment]) {
        perror("Segment Dump Error: Failed to write segment to file: ");
        destroy_assembler(&assembler);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
}