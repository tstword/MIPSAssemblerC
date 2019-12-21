/**
 * @file: mipsfhdr.h
 *
 * @purpose: Defines the necessary functions and structures for creating object files.
 *
 * Each object file contains a file header which is used to determine
 * the endianness of the system that created the file and the number
 * of sections within the file itself.
 *
 * Each section corresponds to a segment and contains the section header which
 * allows the user to determine what segment the following bytes are used for
 * and how many bytes the segment contains.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (11/4/2019)
 **/

#ifndef MIPSFHDR_H
#define MIPSFHDR_H

#include <stdint.h>
#include "assembler.h"

struct MIPS_file_header {
    uint8_t m_magic[4];
    uint8_t m_endianness;
    uint8_t m_version;
    uint8_t m_shnum;
    uint8_t m_padding[1];
};

struct MIPS_sect_header {
    uint8_t sh_segment;
    uint8_t sh_padding[3];
    uint32_t sh_offset;
    uint32_t sh_size;
};

void write_object_file(struct assembler *, const char *);
void dump_segment(struct assembler *, segment_t, const char *);

#endif