#include <string.h>
#include <stdio.h>
#include <hiredis.h>

#include "working_queue.h"

#define QUEUE "working_queue"

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 6379


static redisContext *context;

int init_queue()
{
        context = redisConnect(SERVER_HOST,SERVER_PORT);

        if(context != NULL && context->err){
                printf("Error: %s\n", context->errstr);

        }

        return context->err;
}


int close_queue()
{
        redisFree(context);
        return 0;
}

void enqueue(char *url)
{
        redisCommand(context, "rpush " QUEUE " %s", url);
}

void dequeue(char *url)
{
        redisReply *reply;

        reply = redisCommand(context, "lpop " QUEUE);
        strcpy(url, reply->str);
        freeReplyObject(reply);
}

long long sizeof_queue()
{
        redisReply *reply;
        long long size;

        reply = redisCommand(context, "LLEN " QUEUE);
        size = reply->integer;
        freeReplyObject(reply);

        return size;
}
