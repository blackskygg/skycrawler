#ifndef HASH_H

#define HASH_H

int init_hash();
void lookup_hash(unsigned long index, char *value, const char *name);
void add_to_hash(unsigned long index, const char *value, const char *name);
void close_hash();

#endif
