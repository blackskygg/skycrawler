#ifndef ZSET_H

#define ZSET_H

#define ZSET_MAX_INTER 16

int init_zset();
long long sizeof_zset(const char *name);
void add_to_zset(const char *value, const char *name);
void inter_zset(const char* namev[], size_t namec, const char *newset);
void clear_zset(const char *name);
void get_zset(char *value, const char *name);
void select_zsetdb(int db);
void close_zset();

#endif
