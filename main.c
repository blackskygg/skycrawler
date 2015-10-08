#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iconv.h>
#include <sys/types.h>
#include <regex.h>
#include <pthread.h>
#include <openssl/err.h>

#include <gumbo.h>
#include <curl/curl.h>

#include "data_struct/queue.h"
#include "data_struct/set.h"
#include "data_struct/zset.h"
#include "data_struct/hash.h"

#include "extractor.h"
#include "indexbuilder.h"

#define MAX_URL_LEN  64*1024
#define MAX_PAGE_LEN  64*1024*1024
#define MAX_TITLE_LEN 1024
#define MAX_PAGE_NUM  100000
#define MAX_THREAD_NUM 32

#define CONNECTTIMEOUT_MS 1000
#define TIMEOUT_MS        2000

#define SOURCE_FILE "sources.txt"
#define END_FLAG    "end"

#define WORKING_QUEUE "working_queue"
#define VISITED_SET   "visited_set"

#define PARSE_THRESHOLD 172

#define CHARSET_REGEX \
        "<meta[^>]*(http-equiv[^>]*content-type[^>]*content[^>]*text/html[^>]*){0,1}charset=\"{0,1}([a-z0-9-]*)[^>]*>"

typedef struct{
        char*  content;
        char  charset[32];
        char*  url;
        size_t size;
}page_t;

void *thread_main(void *dump);
void search_for_links(GumboNode* node);
void process_page(page_t *page);
void crawl();
void initialize();
void release();
int is_good_url(char *url);

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
size_t header_callback(char *buffer, size_t size, size_t nitems, void *result);

long long npage; //number of pages processec
unsigned long page_index; //the current index
int  furthermore; //indicate if we are going to add more links

pthread_t threads[MAX_THREAD_NUM];
pthread_mutex_t mutex;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *page)
{
        size_t total;

        total = size*nmemb;

        //if the page is to large, truncate it
        if(((page_t *)page)->size + total <= MAX_PAGE_LEN - 1)
                memcpy(((page_t *)page)->content+((page_t *)page)->size, buffer, total);

        ((page_t *)page)->size += total;

        return total;

}

//filter url by inspecting the header
size_t filter_callback(char *buffer, size_t size, size_t nitems, void *result)
{
        size_t total;

        total = size*nitems;

        if(!strncasecmp(buffer, "Content-Type: text/html", 23))
                *(int *)result = 1;

        return total;
}

//determin charset and filter non-text/html links
size_t header_callback(char *buffer, size_t size, size_t nitems, void *result)
{
        size_t total;
        char charset[64];
        total = size*nitems;

        if(!strncasecmp(buffer, "Content-Type:", 13)){
                if(1 == sscanf(buffer, "Content-Type: text/html; charset=%s", charset)){
                        strcpy((char *)result, charset);
                }
                if(strncasecmp(buffer, "Content-Type: text/html", 23)){
                        return 0;
                }
        }

        return total;
}

void initialize()
{
        char url[MAX_URL_LEN];
        FILE *fsource = fopen(SOURCE_FILE, "r");

        if((0 != init_queue()) || (0 != init_set()) ||
           (0 != init_hash()) || (0 != init_zset())||
           (0 != init_index_builder())) {
                exit(EXIT_FAILURE);
        }

        curl_global_init(CURL_GLOBAL_ALL);
        npage = 0;
        furthermore = 1;

        for(;;){
                fscanf(fsource, "%s", url);
                if((strcmp(url, END_FLAG) != 0) && is_good_url(url)){
                        add_to_set(url, VISITED_SET);
                        enqueue(url, WORKING_QUEUE);
                }else{
                        break;
                }
        }

        fclose(fsource);

}

void release()
{
        curl_global_cleanup();

        close_queue();
        close_set();
        close_hash();
        close_zset();

        close_index_builder();

        puts("exited normally\n");
}

//judge whether the url is linked to an html
//and follow the redirection
inline int is_good_url(char *url)
{
        //it should be a http or https url
        if(strstr(url, "http://") != url && strstr(url, "https://") != url)
                return 0;
        else
                return 1;
}

void search_for_links(GumboNode* node)
{
        GumboAttribute* href;
        GumboVector* children;
        char url[MAX_URL_LEN];

        if (node->type != GUMBO_NODE_ELEMENT) {
                return;
        }

        if (node->v.element.tag == GUMBO_TAG_A &&
            (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {

                int is_exist;

                pthread_mutex_lock(&mutex);
                is_exist = lookup_set(href->value, VISITED_SET);
                pthread_mutex_unlock(&mutex);

                if(!is_exist && furthermore){
                        strcpy(url, href->value);
                        if(is_good_url(url)) {

                                pthread_mutex_lock(&mutex);
                                add_to_set(url, VISITED_SET);
                                enqueue(url, WORKING_QUEUE);
                                //check if we have collected enough links
                                if(sizeof_queue(WORKING_QUEUE) + npage >= MAX_PAGE_NUM) {
                                        furthermore = 0;
                                        pthread_mutex_unlock(&mutex);
                                        return ;
                                }
                                pthread_mutex_unlock(&mutex);
                        }
                }
        }

        children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
                search_for_links((GumboNode*)(children->data[i]));
        }
}

void determin_charset(page_t* page)
{
        regex_t preg;
        regmatch_t pmatch[3];

        assert(!regcomp(&preg, CHARSET_REGEX, REG_ICASE|REG_EXTENDED));
        if(REG_NOMATCH != regexec(&preg, page->content, 3, pmatch, 0)) {
                assert(pmatch[2].rm_so != -1);

                strncpy(page->charset, page->content+pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
                page->charset[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';
        }

        regfree(&preg);
}

//page content here should end with '\0'
void process_page(page_t *page)
{
        char title[MAX_TITLE_LEN];

        //find out the charset of the page
        if(0 == strcmp(page->charset, "not set")){
                determin_charset(page);
        }


        //ignore the unknown pages
        if(0 != strcasecmp(page->charset, "not set") ){
                GumboOutput* output;
                char *buffer = malloc(MAX_PAGE_LEN);

                assert(buffer);

                if(0 != strcasecmp(page->charset, "utf-8")){

                        iconv_t *converter;
                        char *inp, *outp;
                        size_t inbytes, outbytes;

                        if((iconv_t)-1 == (converter = iconv_open("utf-8", page->charset))){
                                printf("error charset %s is not supported\n", page->charset);
                                free(buffer);
                                return;
                        }

                        inp = page->content;
                        outp = buffer;
                        inbytes = page->size;
                        outbytes = MAX_PAGE_LEN - 1;

                        iconv(converter, &inp, &inbytes, &outp, &outbytes);
                        buffer[MAX_PAGE_LEN] = '\0';
                        strncpy(page->content, buffer, MAX_PAGE_LEN - 1);
                        page->content[MAX_PAGE_LEN] = '\0';
                        page->size = outp - buffer;

                        iconv_close(converter);

                }

                output = gumbo_parse_with_options(&kGumboDefaultOptions, page->content, page->size);

                //extract content, title etc;
                extract(output, title, buffer);

                pthread_mutex_lock(&mutex);
                build_index(page_index, page->url, title, buffer);
                page_index++;
                pthread_mutex_unlock(&mutex);

                if(furthermore)
                        search_for_links(output->root);

                gumbo_destroy_output(&kGumboDefaultOptions, output);
                free(buffer);

        }


}

void crawl()
{
        char url[MAX_URL_LEN], **new_url;
        page_t page;
        CURL *curl;
        long rspcode;
        size_t queue_size;

        assert(page.content = malloc(MAX_PAGE_LEN));
        page.url = url;

        pthread_mutex_lock(&mutex);
        curl  = curl_easy_init();
        pthread_mutex_unlock(&mutex);

        //       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &page.charset);

        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, CONNECTTIMEOUT_MS);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, TIMEOUT_MS);


        while(1){

                pthread_mutex_lock(&mutex);
                if(0 != dequeue(url, WORKING_QUEUE) || npage >= MAX_PAGE_NUM){
                        pthread_mutex_unlock(&mutex);
                        break;
                }
                pthread_mutex_unlock(&mutex);

                page.size = 0;
                strcpy(page.charset, "not set");

                curl_easy_setopt(curl, CURLOPT_URL, url);
                page.content[page.size] = '\0';

                if(CURLE_OK != curl_easy_perform(curl))
                        continue;

                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rspcode);
                switch(rspcode) {
                case 301:
                        curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, new_url);
                        strcpy(page.url, *new_url);
                 //fall through
                case 200:
                        break;

                default:
                        printf("rspcode: %d\n", rspcode);
                        continue;
                }

                process_page(&page);

                pthread_mutex_lock(&mutex);
                npage++;
                pthread_mutex_unlock(&mutex);

        }

        free(page.content);
        curl_easy_cleanup(curl);
}

void *thread_main(void *dump)
{
        crawl();
        pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
        int num_threads = 0;
        pthread_attr_t attr;
        int retval = 0;

        if(argc == 3){

                initialize();

                pthread_mutex_init(&mutex, NULL);
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

                num_threads = atoi(argv[1]);
                page_index = atol(argv[2]);

                if(num_threads > MAX_THREAD_NUM) {
                        printf("oh! that's too many!(larger than %d\n", MAX_THREAD_NUM);
                        retval = -1;
                }

                for(int i = 0; i < num_threads; ++i) {
                        if(0 != pthread_create(&threads[i], &attr, thread_main, NULL)) {
                                printf("create threads failed!\n");
                                retval = -1;
                        }
                }


                for(int i = 0; i < num_threads; ++i)
                        pthread_join(threads[i], NULL);

                release();

        } else {
                printf("please specify the number of threads and the starting index(eg. \"crawler 8 2048\").\n");
                return -1;
        }

exit:
        pthread_attr_destroy(&attr);
        pthread_mutex_destroy(&mutex);
        return retval;
}
