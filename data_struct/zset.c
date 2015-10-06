#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <hiredis.h>

#include "zset.h"
#include "db_server.h"

#define MAX_COMMAND_LEN 256

static redisContext *context;

int init_zset()
{
        context = redisConnect(SERVER_HOST,SERVER_PORT);

        if(context != NULL && context->err){
                printf("Error: %s\n", context->errstr);

        }

        return context->err;
}

void close_zset()
{
        redisFree(context);
}

long long sizeof_set(const char *name)
{
        redisReply *reply;
        long long     size;

        reply = redisCommand(context, "zcard %s", name);
        size = reply->integer;
        freeReplyObject(reply);

        return size;
}

void add_to_zset(const char *keyword, unsigned long index)
{
        redisCommand(context, "zincrby %s 1 %ld", keyword, index);
}

void inter_zset(const char* namev[], size_t namec, const char *newset)
{
        char command[MAX_COMMAND_LEN]={"zinterstore "};
        char namec_str[3];

        assert(namec <= 8);
        strcat(command, newset);

        strcat(command, " ");
        sprintf(namec_str, "%ld", namec);
        strcat(command, namec_str);


        for(int i = 0; i < namec; ++i) {
                strcat(command, " ");
                strcat(command, namev[i]);
        }

        redisCommand(context, command);
}
