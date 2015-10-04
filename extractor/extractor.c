#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <gumbo.h>

#define MIN_CONTENT_LEN 20

int find_title(const GumboNode* root, char *title, int length)
{
        assert(root->type == GUMBO_NODE_ELEMENT);
        assert(root->v.element.children.length >= 2);

        const GumboVector* root_children = &root->v.element.children;
        GumboNode* head = NULL;
        for (int i = 0; i < root_children->length; ++i) {
                GumboNode* child = root_children->data[i];
                if (child->type == GUMBO_NODE_ELEMENT &&
                    child->v.element.tag == GUMBO_TAG_HEAD) {
                        head = child;
                        break;
                }
        }
        assert(head != NULL);

        GumboVector* head_children = &head->v.element.children;
        for (int i = 0; i < head_children->length; ++i) {
                GumboNode* child = head_children->data[i];
                if (child->type == GUMBO_NODE_ELEMENT &&
                    child->v.element.tag == GUMBO_TAG_TITLE) {
                        if (child->v.element.children.length != 1) {
                                return 0;
                        }
                        GumboNode* title_text = child->v.element.children.data[0];
                        assert(title_text->type == GUMBO_NODE_TEXT || title_text->type == GUMBO_NODE_WHITESPACE);
                        strncpy(title, title_text->v.text.text, length);
                        return 1;
                }
        }
        return 0;
}

void extract_text(GumboNode* node, char *buffer, int *pos){
        if (node->type == GUMBO_NODE_TEXT) {
                size_t pure_len, len;

                for(pure_len = len = 0; node->v.text.text[len] != '\0'; len++){
                        if(!isspace(node->v.text.text[len]))
                                pure_len++;
                }

                //ignore short lines
                if(pure_len >= MIN_CONTENT_LEN){
                        strcpy(buffer+*pos, node->v.text.text);
                        *pos += strlen(node->v.text.text);
                }

        } else if (node->type == GUMBO_NODE_ELEMENT &&
                   node->v.element.tag != GUMBO_TAG_SCRIPT &&
                   node->v.element.tag != GUMBO_TAG_STYLE) {

                int i;
                GumboVector* children = &node->v.element.children;
                for (i = 0; i < children->length; ++i) {
                        int prev_pos = *pos;

                        extract_text((GumboNode *)children->data[i], buffer, pos);
                        if (i != 0 && (*pos - prev_pos) > 0) {
                                buffer[*pos]='\n';
                                buffer[*pos + 1] = '\0';
                                (*pos)++;
                        }
                }

        }
}

/*
static void read_file(FILE* fp, char** output, int* length) {
        struct stat filestats;
        int fd = fileno(fp);
        fstat(fd, &filestats);
        *length = filestats.st_size;
        *output = malloc(*length + 1);
        int start = 0;
        int bytes_read;
        while ((bytes_read = fread(*output + start, 1, *length - start, fp))) {
                start += bytes_read;
        }
}


int main(int argc, char **argv)
{
        FILE *fp = fopen(argv[1], "r");
        char *output, title[1000], content[64000];
        int  length;
        int  pos;

        read_file(fp, &output, &length);
        output[length] = '\0';
        GumboOutput* Goutput = gumbo_parse(output);

        find_title(Goutput->root, title, 1000);
        title[999] = '\0';
        printf("title: %s\n", title);

        pos = 0;
        extract_text(Goutput->root, content, &pos);
        content[63999] = '\0';
        printf("content: \n%s", content);

        return 0;

}
*/
