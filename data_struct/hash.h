#ifndef HASH_H

#define HASH_H

int init_hash();
void lookup_hash(const char *key, char *value, const char *name);
void add_to_hash(const char *key, const char *value, const char *name);
void close_hash();
void select_hashdb(int db);

#endif
