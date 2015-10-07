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

void select_hashdb(int db)
{
        redisCommand(context, "select %d", db);
}

void lookup_hash(const char *key, char *value, const char *name)
{
        redisReply *reply;

        reply = redisCommand(context, "hget %s %s", name, key);
        strcpy(value, reply->str);
        freeReplyObject(reply);
}

void add_to_hash(const char *key, const char *value, const char *name)
{
        redisCommand(context, "hset %s %s %s", name, key, value);
}
