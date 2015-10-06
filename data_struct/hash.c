#include <string.h>
#include <stdio.h>
#include <hiredis.h>

#include "set.h"
#include "db_server.h"

static redisContext *context;

int init_hash()
{
        context = redisConnect(SERVER_HOST,SERVER_PORT);

        if(context != NULL && context->err){
                printf("Error: %s\n", context->errstr);

        }

        return context->err;
}

void close_hash()
{
        redisFree(context);
}

void lookup_hash(unsigned long index, char *value, const char *name)
{
        redisReply *reply;

        reply = redisCommand(context, "hget %s %ld", name,  index);
        strcpy(value, reply->str);
        freeReplyObject(reply);
}

void add_to_hash(unsigned long index, const char *value, const char *name)
{
        redisCommand(context, "hset %s %ld %s", name, index, value);

}
