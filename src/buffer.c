#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "buffer.h"
#include "utility.h"

// @refactor is fopen the right way to do this?
// @feat add backup files
Buffer *buf_open(const char *path, int width, int height) {
    Buffer *b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->width = width;
    b->height = height;

    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        b->data = malloc(1);
        b->data[0] = '\0';
        b->size = b->capacity = 1;
        b->saved = false;
    } else {
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        b->data = malloc(size + 1);
        fread(b->data, 1, size, fp);
        b->data[size] = '\0';
        b->size = b->capacity = size + 1;

        b->saved = true;
        fclose(fp);
    }

    return b;
}

void buf_set_dimensions(Buffer *b, int width, int height) {
    b->width = width;
    b->height = height;
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
