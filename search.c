#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <friso/friso_API.h>
#include <friso/friso.h>

#include "data_struct/hash.h"
#include "data_struct/zset.h"

#include "search.h"

#define SEARCH_ZSET "search_zset"
#define INTER_ZSET "inter_zset"
#define URL_HASH  "url_hash"

#define CONTENT_DB 2
#define TITLE_DB   1
#define DEFAULT_DB 0

#define MAX_URL_LEN 64*1024
#define MAX_TITLE_LEN 1024
#define MAX_KEY_LEN   16

#define FRISO_PATH "friso/friso.ini"

#define GET_BIT(x, n) ((x) & (1U << (n)))
#define SET_BIT(x, n) ((x) = (x) | (1U << (n)))

int combo_search(char (*namev)[16], size_t namec, size_t num);
unsigned int next_k_bit(unsigned int x);

int main(int argc, char* argv[])
{
        if(argc <= 1)
                return 0;

        char title[MAX_TITLE_LEN], query[ZSET_MAX_INTER][MAX_KEY_LEN];
        size_t  nquery;
        unsigned long index;

        friso_t friso;
        friso_config_t config;
        friso_task_t task;


        init_hash();
        init_zset();

//init friso
        config = friso_new_config();
        friso = friso_new();
        task = friso_new_task();

        if(friso_init_from_ifile(friso, config, FRISO_PATH) != 1) {
                printf("fail to initialize friso and config.");
                goto clear;
        }
        config->clr_stw = 0;

//process search string
        select_zsetdb(DEFAULT_DB);
        for(int i = 1; i < argc; ++i) {
                friso_set_text(task, argv[i]);
                while((config->next_token(friso, config, task)) != NULL) {
                        add_to_zset(task->token->word, SEARCH_ZSET);
                }

        }

        printf("\nwill search the db using the combination of the following key words;\n");
        nquery = 0;
        while(sizeof_zset(SEARCH_ZSET) > 0) {
                if(nquery + 1 > ZSET_MAX_INTER) {
                        printf("\ntoo many key words, your query string was truncated\n");
                        break;
                }
                get_zset(query[nquery], SEARCH_ZSET);
                printf("%s ", query[nquery]);
                nquery++;
        }
        putchar('\n');


//search in title db
        select_zsetdb(TITLE_DB);
        printf("\nhere are the pages whose title contains all or part of your key words:\n");
        for(int i = nquery; i >= 1; --i) {
                combo_search(query, nquery, i);
        }


//search in content db
        select_zsetdb(CONTENT_DB);
        printf("\nhere are the pages whose contents contains all or part of your key words:\n");
        for(int i = nquery; i >= 1; --i) {
                combo_search(query, nquery, i);
        }

clear:
        close_hash();
        close_zset();
        friso_free_config(config);
        friso_free(friso);

        return 0;
}

int combo_search(char (*namev)[16], size_t namec, size_t num)
{
        unsigned int bitmap = 0, end_bitmap = 0;
        char *query[ZSET_MAX_INTER], url[MAX_URL_LEN], value[16];
        size_t  nquery = 0;

        for(int i = 0; i < num; ++i){
                SET_BIT(bitmap, i);
                SET_BIT(end_bitmap, (namec - 1) - i);
        }

        while(1) {
                nquery = 0;
                for(int i = 0; i < namec; ++i) {
                        if(GET_BIT(bitmap, i)) {
                                query[nquery] = namev[i];
                                nquery++;
                        }
                }

                inter_zset(query, nquery, INTER_ZSET);
                while(sizeof_zset(INTER_ZSET) > 0) {
                        get_zset(value, INTER_ZSET);
                        lookup_hash(value, url, URL_HASH);
                        printf("\nurl: %s\n", url);
                }

                if(bitmap == end_bitmap)
                        break;

                bitmap = next_k_bit(bitmap);
        };
}


//get the next integer with k bits on
//see http://realtimecollisiondetection.net/blog/?p=78 for derivation
unsigned int next_k_bit(unsigned int x)
{
        unsigned int b, t, c, m;

        b = x & -x;
        t = x + b;
        c = t ^ x;
        m = (c >> 2) / b;

        return t | m;
}
