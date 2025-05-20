#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "buffer.h"
#include "utility.h"

Buffer *buf_create_from_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    Buffer *b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->data = malloc(size + 1);
    fread(b->data, 1, size, fp);
    b->data[size] = '\0';
    b->size = b->capacity = size + 1;

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->saved = true;

    fclose(fp);
    return b;
}

Buffer *buf_create_empty(const char *path) {
    Buffer *b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->data = malloc(1);
    b->data[0] = '\0';
    b->size = b->capacity = 1;

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->saved = false;

    return b;
}

// @refactor is fopen the right way to do this?
// @feat add backup files
Buffer *buf_open(const char *path) {
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        return buf_create_empty(path);
    } else {
        fclose(fp);
        return buf_create_from_file(path);
    }
}

// @feat add backup files
void buf_save(Buffer *b) {
    append_newline_maybe(b);

    size_t len = strlen(b->data);
    FILE *fp = fopen(b->path, "w");
    fwrite(b->data, 1, len, fp);
    b->saved = true;
    fclose(fp);
}

void buf_kill(Buffer *b) {
    free(b->data);
    free(b->path);
    free(b);
}
