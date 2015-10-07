#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <friso/friso_API.h>
#include <friso/friso.h>

#include "indexbuilder.h"
#include "data_struct/hash.h"
#include "data_struct/zset.h"

#define DEFAULT_DB 0
#define TITLE_DB 1
#define CONTENT_DB 2
#define URL_HASH "url_hash"

#define FRISO_PATH "friso/friso.ini"

static friso_t friso;
static friso_config_t config;

int init_index_builder()
{
        config = friso_new_config();
        friso = friso_new();

        if(friso_init_from_ifile(friso, config, FRISO_PATH) != 1) {
                printf("fail to initialize friso and config.");
                friso_free_config(config);
                friso_free(friso);
                return -1;
        }

        return 0;
}

void close_index_builder()
{
        friso_free_config(config);
        friso_free(friso);
}
void build_index(unsigned long index, const char *url, const char *title, const char *content)
{
        char str_index[32];
        friso_task_t task;

        task = friso_new_task();
        sprintf(str_index,"%lu", index);

//process url
        select_zsetdb(DEFAULT_DB);
        add_to_hash(str_index, url, URL_HASH);

//process title, stop words are kept
        config->clr_stw = 0;
        friso_set_text(task, title);
        select_zsetdb(TITLE_DB);
        while((config->next_token(friso, config, task)) != NULL) {
                add_to_zset(str_index, task->token->word);
        }

//process content, stop words are ignored
        config->clr_stw = 1;
        friso_set_text(task, content);
        select_zsetdb(CONTENT_DB);
        while((config->next_token(friso, config, task)) != NULL) {
                add_to_zset(str_index, task->token->word);
        }

//switch back to default db
        select_zsetdb(DEFAULT_DB);
        friso_free_task(task);
}
