#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdlib.h>
#include <stdint.h>

/* Marco definitions... */
#define SEGMENT_NULL        0x0
#define SEGMENT_TEXT        0x1
#define SEGMENT_DATA        0x2

#define OFFSET_BYTE         0x1
#define OFFSET_HALFWORD     0x2
#define OFFSET_WORD         0x4

#define SYMBOL_UNDEFINED    0x0
#define SYMBOL_DEFINED      0x1
#define SYMBOL_DOUBLY       0x2

/* Type definitions */
typedef uint32_t offset_t;
typedef uint32_t segment_t;
typedef uint16_t datasize_t;
typedef uint8_t  symstat_t; 

/* Forward declare instruction_node */
struct instruction_node;

struct symbol_table_entry {
    char *key;                              /* Identifier for label */
    symstat_t status;                       /* Indicated status of symbol: defined, undefined, doubly */
    offset_t offset;                        /* Offset from segment */
    segment_t segment;                      /* Segment */
    datasize_t datasize;                    /* Size of the data */
    struct instruction_node *instr_list;    /* List of instructions that rely on this symbol that hasn't been defined */
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
struct symbol_table_entry *get_symbol_table(struct symbol_table *, const char *);
void destroy_symbol_table(struct symbol_table **);

void print_symbol_table(struct symbol_table *);

#endif