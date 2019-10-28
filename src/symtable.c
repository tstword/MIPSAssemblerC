#include "symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // ??? 

/* String hash function used for the key in the symbol table... */
size_t djb2hash(const char *str) {
    const unsigned char *key = (const unsigned char *)str;
    size_t hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/* Create symbol table and return to user */
struct symbol_table *create_symbol_table() {
    struct symbol_table *symtab = malloc(sizeof(struct symbol_table));
    
    symtab->buckets = calloc(32, sizeof(struct symbol_table_entry));
    symtab->bucket_size = 32;
    symtab->length = 0;

    return symtab;
}

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

struct symbol_table_entry *insert_symbol_table(struct symbol_table *symtab, const char *key) {
    struct symbol_table_entry *item = malloc(sizeof(struct symbol_table_entry));
    
    item->key = strdup(key);
    item->status = SYMBOL_UNDEFINED;
    item->offset = 0x00;
    item->segment = 0x00;
    item->datasize = 0x00;
    item->instr_list = NULL;
    item->next = NULL;

    size_t index = djb2hash(key) % symtab->bucket_size;

    insert_at_index_st(symtab, index, item);
    
    if(((float)symtab->length) / symtab->bucket_size >= 0.7f) {
        struct symbol_table_entry **old_buckets = symtab->buckets;
        size_t old_size = symtab->bucket_size;
        
        symtab->bucket_size <<= 1;
        symtab->length = 0;
        symtab->buckets = calloc(symtab->bucket_size, sizeof(struct symbol_table_entry));

        for(size_t i = 0; i < old_size; ++i) {
            struct symbol_table_entry *head = old_buckets[i];
           
           while(head != NULL) {
                struct symbol_table_entry *next_head = head->next;
                head->next = NULL;
                index = djb2hash(head->key) % symtab->bucket_size;
                insert_at_index_st(symtab, index, head);
                head = next_head;
            }
        }

		free(old_buckets);
    }

    return item;
}

struct symbol_table_entry *get_symbol_table(struct symbol_table *symtab, const char *key) {
    size_t index = djb2hash(key) % symtab->bucket_size;
    struct symbol_table_entry *head = symtab->buckets[index];

    while(head != NULL) {
        if(strcmp(key, head->key) == 0) return head;
        head = head->next;
    }

    return NULL;
}

void destroy_symbol_table(struct symbol_table **symtabp) {
    struct symbol_table *symtab = *symtabp;

    /* Destory all pairs in the hashtable */
    for(size_t i = 0; i < (symtab)->bucket_size; ++i) {
        struct symbol_table_entry *head = symtab->buckets[i];
        while(head != NULL) {
            struct symbol_table_entry *next_item = head->next;
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

void print_symbol_table(struct symbol_table *symtab) {
    printf("===== Symbol Table =====\n");
    size_t bucketsUsed = 0;
    for(size_t i = 0; i < symtab->bucket_size; ++i) {
        struct symbol_table_entry *head = symtab->buckets[i];
        if(head != NULL) ++bucketsUsed;
        while(head != NULL) {
            const char *def[3] = { "UNDEFINED", "DEFINED", "DOUBLY" };
            printf("[ %-20s | 0x%08X | 0x%02X | 0x%02X | %-10s ]---> ", head->key, head->offset, head->segment, head->datasize, def[head->status]);
            if(head->next == NULL) printf("\n");
            head = head->next;
        }
    }
    //printf("Length: %ld\nBucket Used: %ld\nBucket Size: %ld\n", symtab->length, bucketsUsed, symtab->bucket_size);
}