#include <string.h>
#include <stdio.h>
#include <hiredis.h>

#include "visited_set.h"

#define SET "visited_set"

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 6379


static redisContext *context;


int init_set()
{
        context = redisConnect(SERVER_HOST,SERVER_PORT);

        if(context != NULL && context->err){
                printf("Error: %s\n", context->errstr);

        }

        return context->err;
}

void close_set()
{
        redisFree(context);
}

int sizeof_set()
{
        redisReply *reply;
        long long     size;

        reply = redisCommand(context, "scard " SET);
        size = reply->integer;
        freeReplyObject(reply);

        return size;

}
void add_to_set(const char *url)
{
        redisCommand(context, "sadd " SET " %s", url);
}

void del_form_set(const char *url)
{
        redisCommand(context, "srem " SET " %s", url);
}

int lookup_set(const char *url)
{
        redisReply *reply;
        int result;

        reply = redisCommand(context, "sismember " SET " %s", url);
        result = reply->integer;
        freeReplyObject(reply);

        return result;
}
