#ifndef QUEUE_H

#define QUEUE_H

int init_queue();
long long sizeof_queue(const char *name);
void enqueue(const char *url, const char *name);
int dequeue(char *url, const char *name);
void close_queue();


#endif
