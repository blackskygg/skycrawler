#ifndef VISITED_SET_H

#define VISITED_SET_H

int init_set();
int lookup_set(char *url);
void add_to_set(char *url);
void del_from_set(char *url);
void close_set();

#endif
