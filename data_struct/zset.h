#ifndef ZSET_H

#define ZSET_H

int init_zset();
long long sizeof_set(const char *name);
void add_to_zset(const char *keyword, unsigned long index);
void inter_zset(const char* namev[], size_t namec, const char *newset);
void clear_zset(const char *name);
void close_zset();

#endif
