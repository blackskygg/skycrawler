#include <string.h>
#include <stdio.h>
#include <hiredis.h>

#include "set.h"
#include "db_server.h"

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

int sizeof_set(const char *name)
{
        redisReply *reply;
        long long     size;

        reply = redisCommand(context, "scard %s", name);
        size = reply->integer;
        freeReplyObject(reply);

        return size;

}
void add_to_set(const char *url, const char *name)
{
        redisCommand(context, "sadd %s %s", name, url);
}

void del_form_set(const char *url, const char *name)
{
        redisCommand(context, "srem %s %s", name, url);
}

int lookup_set(const char *url, const char *name)
{
        redisReply *reply;
        int result;

        reply = redisCommand(context, "sismember %s %s", name,  url);
        result = reply->integer;
        freeReplyObject(reply);

        return result;
}
