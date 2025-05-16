#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "buffer.h"
#include "utility.h"

Buffer *buf_create_from_file(const char *path) {
    Buffer *b = NULL;
    size_t size = 0;
    FILE *fp = NULL;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->data = malloc(size + 1);
    fread(b->data, 1, size, fp);
    b->data[size] = '\0';

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->saved = true;

    fclose(fp);
    return b;
}

Buffer *buf_create_empty(const char *path) {
    Buffer *b = NULL;

    b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->data = malloc(1);
    b->data[0] = '\0';

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->saved = false;

    return b;
}

Buffer *buf_open(const char *path) {
    FILE *fp = NULL;

    /* TODO is fopen the right way to do this? */
    /*      and is this function abstraction even good? */

    fp = fopen(path, "r");
    if (fp == NULL) {
        return buf_create_empty(path);
    } else {
        fclose(fp);
        return buf_create_from_file(path);
    }
}

void buf_save(Buffer *b) {
    FILE *fp = NULL;
    size_t len = 0;

    append_newline_maybe(b);

    len = strlen(b->data);
    fp = fopen(b->path, "w");
    fwrite(b->data, 1, len, fp);
    b->saved = true;
    fclose(fp);
}

void buf_kill(Buffer *b) {
    free(b->data);
    free(b->path);
    free(b);
}
