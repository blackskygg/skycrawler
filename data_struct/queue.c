#include <string.h>
#include <stdio.h>
#include <hiredis.h>

#include "queue.h"
#include "db_server.h"

static redisContext *context;

int init_queue()
{
        context = redisConnect(SERVER_HOST,SERVER_PORT);

        if(context != NULL && context->err){
                printf("Error: %s\n", context->errstr);

        }

        return context->err;
}


void close_queue()
{
        redisFree(context);
}

void enqueue(const char *url, const char *name)
{
        redisCommand(context, "rpush %s %s", name, url);
}

int dequeue(char *url, const char *name)
{
        redisReply *reply;

        reply = redisCommand(context, "lpop %s", name);
        if(reply->type == REDIS_REPLY_NIL) {
                freeReplyObject(reply);
                return -1;
        }
        strcpy(url, reply->str);
        freeReplyObject(reply);

        return 0;
}

long long sizeof_queue(const char *name)
{
        redisReply *reply;
        long long size;

        reply = redisCommand(context, "llen %s", name);
        size = reply->integer;
        freeReplyObject(reply);

        return size;
}
