#ifndef INDEXBUILDER_H

#define INDEXBUILDER_H

int init_index_builder();
void build_index(unsigned long index, const char *url, const char *title, const char *content);
void close_index_builder();

#endif
