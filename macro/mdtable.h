#ifndef MDTABLE_H
#define MDTABLE_H

struct MDT_entry {
    char             *macro_name;
    struct MDT_entry *prev_entry;
    uint32_t         param_count;
};

struct MDT {
    struct MDT_entry *rear;
    char             *memory;
};

struct mdtab_entry *mdtab_search(c)

#endif