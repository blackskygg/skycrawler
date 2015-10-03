#ifndef SET_H

#define SET_H

int init_set();
int lookup_set(const char *url, const char *name);
int sizeof_set(const char *name);
void add_to_set(const char *url, const char *name);
void del_from_set(const char *url, const char *name);
void close_set();

#endif
