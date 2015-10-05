#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iconv.h>
#include <sys/types.h>
#include <regex.h>

#include <gumbo.h>
#include <curl/curl.h>

#include "data_struct/queue.h"
#include "data_struct/set.h"
#include "extractor.h"


#define MAX_URL_LEN  64*1024
#define MAX_PAGE_LEN 64*1024*1024
#define MAX_PAGE_NUM  30

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
        size_t size;
}page_t;

void search_for_links(GumboNode* node);
void process_page(page_t *page);
void crawl();
void initialize();
void release();
int is_good_url(char *url);

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
size_t filter_callback(char *buffer, size_t size, size_t nitems, void *result);
size_t charset_callback(char *buffer, size_t size, size_t nitems, void *result);

long long npage; //number of pages processec
int  furthermore; //indicate if we are going to add more links

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

size_t charset_callback(char *buffer, size_t size, size_t nitems, void *result)
{
        size_t total;
        char charset[64];
        total = size*nitems;

        if(1 == sscanf(buffer, "Content-Type: text/html; charset=%s", charset)){
                strcpy((char *)result, charset);
        }

        return total;
}

void initialize()
{
        char url[MAX_URL_LEN];
        FILE *fsource = fopen(SOURCE_FILE, "r");

        if((0 != init_queue()) || (0 != init_set()))
                exit(EXIT_FAILURE);

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
        puts("exited normally\n");
}

//judge whether the url is linked to an html
//and follow the redirection
int is_good_url(char *url)
{
        CURL *curl = curl_easy_init();
        long rspcode;
        int  result;
        char **new_url;

        //it should be a http or https url
        if(strstr(url, "http://") != url && strstr(url, "https://") != url)
                return 0;

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);

        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, filter_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, CONNECTTIMEOUT_MS);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, TIMEOUT_MS);

        result = 0;
        if(CURLE_OK != curl_easy_perform(curl) || !result)
                return 0;

        //recursively check the availability of the url
        //302 is disturbing, just forget it
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rspcode);
        if(rspcode == 301){
                curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, new_url);
                strcpy(url, *new_url);

                return is_good_url(url);
        }

        curl_easy_cleanup(curl);

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
                if(!lookup_set(href->value, VISITED_SET) && furthermore){
                        strcpy(url, href->value);
                        if(is_good_url(url)) {
                                add_to_set(url, VISITED_SET);
                                enqueue(url, WORKING_QUEUE);

                                //check if we have collected enough links
                                if(sizeof_queue(WORKING_QUEUE) + npage >= MAX_PAGE_NUM) {
                                        furthermore = 0;
                                        return ;
                                }
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
                system("clear");
                fwrite(page->content, 1, page->size, stdout);
                getchar();
                output = gumbo_parse_with_options(&kGumboDefaultOptions, page->content, page->size);

                //extract content, title etc;
                //the buffer is reused
                extract(output, buffer);

                if(furthermore)
                        search_for_links(output->root);

                gumbo_destroy_output(&kGumboDefaultOptions, output);
                free(buffer);

        }


}

void crawl()
{
        char url[MAX_URL_LEN];
        page_t page;
        CURL *curl;
        long rspcode;

        assert(page.content = malloc(MAX_PAGE_LEN));

        curl  = curl_easy_init();
        //       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);

        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, charset_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &page.charset);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, CONNECTTIMEOUT_MS);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, TIMEOUT_MS);

        while(sizeof_queue(WORKING_QUEUE) > 0 && npage <= MAX_PAGE_NUM){
                dequeue(url, WORKING_QUEUE);

                page.size = 0;
                strcpy(page.charset, "not set");

                curl_easy_setopt(curl, CURLOPT_URL, url);
                page.content[page.size] = '\0';

                if(CURLE_OK != curl_easy_perform(curl)) continue;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rspcode);
                if(rspcode != 200) continue;

                process_page(&page);

                npage++;
        }

        free(page.content);
        curl_easy_cleanup(curl);
}

int main()
{
        initialize();

        crawl();
        release();

        return 0;

}
