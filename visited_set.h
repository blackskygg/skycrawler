#ifndef VISITED_SET_H

#define VISITED_SET_H

int init_set();
int lookup_set(const char *url);
int sizeof_set();
void add_to_set(const char *url);
void del_from_set(const char *url);
void close_set();

#endif
