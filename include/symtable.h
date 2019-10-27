#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdlib.h>
#include <stdint.h>

/* Marco definitions... */
#define ST_NULL             0x0
#define SEGMENT_TEXT        0x1
#define SEGMENT_DATA        0x2

#define OFFSET_BYTE         0x1
#define OFFSET_HALFWORD     0x2
#define OFFSET_WORD         0x4

/* Type definitions */
typedef uint32_t offset_t;
typedef uint32_t segment_t;
typedef uint16_t datasize_t;

/* Symbol Entry Attribute structure */
struct symbol_table_attr {
    offset_t offset;                        /* Offset from segment */
    segment_t segment;                      /* Segment */
    datasize_t datasize;                    /* Size of the data */
};

struct symbol_table_entry {
    char *key;                        /* Identifier for label */
    struct symbol_table_attr value;         /* Contains segment, offset, and size */
    struct symbol_table_entry *next;        /* Pointer to next entry */
};

struct symbol_table {
    struct symbol_table_entry **buckets;    /* Pointer to the array of entries */
    size_t bucket_size;                     /* Total number of buckets */
    size_t length;                          /* Total elements in array */
};

/* Global access to symbol table */
extern struct symbol_table *symbol_table;

/* Function prototypes */
struct symbol_table *create_symbol_table();
struct symbol_table_entry *insert_symbol_table(struct symbol_table *, const char *);
struct symbol_table_attr *get_symbol_table(struct symbol_table *, const char *);
void destroy_symbol_table(struct symbol_table **);

void print_symbol_table(struct symbol_table *);

#endif