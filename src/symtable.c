/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: symtable.c
 *
 * Purpose: Defines the necessary functions to construct a symbol table
 * The symbol table is constructed as a hash table with a load of 70% before
 * percolating. The operation run times are listed below:
 *
 *      Insertion: O(1) expected time
 *      Access:    O(1) expected time
 *
 * Entries in the symbol table can be defined as:
 *      UNDEFINED: Referenced by instruction before being declared
 *      DEFINED:   Declared and defined
 *      DOUBLY:    Multiple definitions (cannot be assembled)
 *
 * The hashing algorithm used in this implementation is the djb2 hashing
 * algorithm
 *
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Segment string array */
const char *segment_string[MAX_SEGMENTS] = { 
    [SEGMENT_TEXT]  = "TEXT" , [SEGMENT_DATA]  = "DATA" ,
    [SEGMENT_KTEXT] = "KTEXT", [SEGMENT_KDATA] = "KDATA",
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: djb2hash
 * Purpose: Computes a hash value from the string
 * @param str -> String to hash
 * @return Returns the hashed value
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
size_t djb2hash(const char *str) {
    const unsigned char *key = (const unsigned char *)str;
    size_t hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: create_symbol_table
 * Purpose: Allocates and initializes symbol table structure
 * @return Address of the allocated symbol table structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct symbol_table *create_symbol_table() {
    struct symbol_table *symtab = (struct symbol_table *)malloc(sizeof(struct symbol_table));
    
    symtab->buckets = (struct symbol_table_entry **)calloc(32, sizeof(struct symbol_table_entry));
    symtab->bucket_size = 32;
    symtab->length = 0;

    return symtab;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: insert_at_index_st
 * Purpose: Inserts new symbol_table entry based on bucket index
 * @param symtab -> Address of the symbol table
 *        index  -> Index of the bucket to insert at
 *        entry  -> Symbol table entry to insert
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void insert_at_index_st(struct symbol_table *symtab, size_t index, struct symbol_table_entry *entry) {
    struct symbol_table_entry *head = symtab->buckets[index];
    ++symtab->length;
    if(head == NULL) {
        symtab->buckets[index] = entry;
        return;
    }
    while(head->next != NULL) head = head->next;
    head->next = entry;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: percolate_symbol_table
 * Purpose: Expands bucket size of the symbol table and moves all entries over
 * if the load facter is at least 70%
 * @param symtab -> Address of the symbol table
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void percolate_symbol_table(struct symbol_table *symtab) {
    float symtab_load = ((float)symtab->length) / symtab->bucket_size;
    
    if(symtab_load >= 0.7f) {
        struct symbol_table_entry **prev_buckets = symtab->buckets;
        size_t prev_size = symtab->bucket_size;
        
        symtab->bucket_size <<= 1;
        symtab->length = 0;
        symtab->buckets = (struct symbol_table_entry **)calloc(symtab->bucket_size, sizeof(struct symbol_table_entry));

        for(size_t i = 0; i < prev_size; ++i) {
            struct symbol_table_entry *head = prev_buckets[i], *next_head;
            size_t index;

            while(head != NULL) {
                next_head = head->next;
                head->next = NULL;
                
                index = djb2hash(head->key) % symtab->bucket_size;
                insert_at_index_st(symtab, index, head);
                
                head = next_head;
            }
        }

		free(prev_buckets);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: insert_symbol_table
 * Purpose: Inserts new symbol into the symbol table and returns the address of 
 * the new entry. Note that a new entry is by default an UNDEFINED symbol.
 * @param symtab -> Address of the symbol table
 *        key    -> Symbol name to insert
 * @return Address of the new entry
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct symbol_table_entry *insert_symbol_table(struct symbol_table *symtab, const char *key) {
    struct symbol_table_entry *item = (struct symbol_table_entry *)malloc(sizeof(struct symbol_table_entry));
    
    item->key = strdup(key);
    item->status = SYMBOL_UNDEFINED;
    item->offset = 0x00;
    item->segment = SEGMENT_TEXT; /* Default is SEGMENT_TEXT */
    item->datasize = 0x00;
    item->instr_list = create_list();
    item->next = NULL;

    size_t index = djb2hash(key) % symtab->bucket_size;

    insert_at_index_st(symtab, index, item);
    
    /* Check symbol table load */
    percolate_symbol_table(symtab);

    return item;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: get_symbol_table
 * Purpose: Search symbol table for a symbol and return the corresponding entry
 * @param symtab -> Address of the symbol table
 *        key    -> Name of the symbol to search
 * @return Address of the entry if found, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct symbol_table_entry *get_symbol_table(struct symbol_table *symtab, const char *key) {
    size_t index = djb2hash(key) % symtab->bucket_size;
    struct symbol_table_entry *head = symtab->buckets[index];

    while(head != NULL) {
        if(strcmp(key, head->key) == 0) return head;
        head = head->next;
    }

    return NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: destroy_symbol_table
 * Purpose: Deallocates symbol table and all of its entries
 * @param symtabp -> Reference to the address of the symbol table
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void destroy_symbol_table(struct symbol_table **symtabp) {
    struct symbol_table *symtab = *symtabp;

    /* Destory all pairs in the hashtable */
    for(size_t i = 0; i < (symtab)->bucket_size; ++i) {
        struct symbol_table_entry *head = symtab->buckets[i];
        while(head != NULL) {
            struct symbol_table_entry *next_item = head->next;
            delete_linked_list(&head->instr_list, LN_VSTATIC);
            free(head->key);
            free(head);
            head = next_item;
        }
    }

	/* Destroy buckets */
	free(symtab->buckets);

    /* Destory hash table */
    free(symtab);

    /* Set to NULL */
    *symtabp = NULL;
}

#ifdef DEBUG

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: print_symbol_table
 * Purpose: Used for debugging puproses. Prints the entries in the symbol
 * table and their corresponding attributes.
 * @param symtab -> Address of the symbol table
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_symbol_table(struct symbol_table *symtab) {
    const char *symtab_status_str[3] = { "UNDEFINED", "DEFINED", "DOUBLY" };

    printf("[ ***** Symbol Table ***** ]\n");

    for(size_t i = 0; i < symtab->bucket_size; ++i) {
        for(struct symbol_table_entry *head = symtab->buckets[i]; head != NULL; head = head->next) {
            printf("[ %-20s | 0x%08X | %-5s | 0x%02X | %-8s ]---> ", head->key, head->offset, segment_string[head->segment], 
                    head->datasize, symtab_status_str[head->status]);
            
            if(head->next == NULL) printf("\n");
        }
    }
}

#endif
