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

void add_to_set(char *url)
{
        redisReply *reply;

        reply = redisCommand(context, "sadd " SET " %s", url);
        freeReplyObject(reply);
}

void del_form_set(char *url)
{
        redisReply *reply;

        reply = redisCommand(context, "srem " SET " %s", url);
        freeReplyObject(reply);
}

int lookup_set(char *url)
{
        redisReply *reply;
        int result;

        reply = redisCommand(context, "srem " SET " %s", url);
        result = reply->integer;
        freeReplyObject(reply);

        return result;
}
