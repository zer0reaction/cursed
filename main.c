#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char u8;
typedef char s8;
typedef unsigned short int u16;
typedef short int s16;
typedef unsigned int u32;
typedef int s32;

typedef struct Line {
    u32 begin;
    u32 end;
} Line;

typedef struct Lines {
    Line *data;
    u32 size;
    u32 capacity;
} Lines;

typedef struct Buffer {
    char *data;

    Lines lines;
} Buffer;

/* line functions */

void lines_append(Lines *lines, const Line line)
{
    if (lines->data == NULL) {
        lines->data = malloc(sizeof(Line) * 1);
        lines->data[0] = line;
        lines->size = 1;
        lines->capacity = 1;
    } else if (lines->size < lines->capacity) {
        lines->data[lines->size] = line;
        lines->size++;
    } else if (lines->size == lines->capacity) {
        lines->capacity *= 2;
        lines->data = realloc(lines->data, sizeof(Line) * lines->capacity);
        lines->data[lines->size] = line;
        lines->size++;
    } else assert("lines_append" && 0);
}

void lines_retokenize(Lines *lines, const char *data)
{
    u32 i = 0;
    Line line = {0};

    lines->size = 0;

    while (1) {
        if (data[i] == '\n' || data[i] == '\0') {
            line.end = i;
            lines_append(lines, line);

            if (data[i] == '\0') break;

            line.begin = i + 1;
            line.end = 0;
        }
        i++;
    }
}

/* buffer functions */

u32 buffer_from_file(Buffer *b, const char *path)
{
    FILE *fp;
    u32 size;

    fp = fopen(path, "r");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    b->data = malloc(size + 1);
    fread(b->data, 1, size, fp);
    b->data[size] = '\0';

    b->lines.data = NULL;
    b->lines.size = 0;
    b->lines.capacity = 0;
    lines_retokenize(&(b->lines), b->data);

    fclose(fp);
    return b->lines.size;
}

void buffer_kill(Buffer *b)
{
    free(b->data);
    free(b->lines.data);
}

int main(int argc, char **argv)
{
    u32 i;
    Buffer b = {0};

    if (argc != 2) return 1;

    buffer_from_file(&b, argv[1]);
    if (!b.lines.size) return 1;
    printf("%u line(s) loaded\n", b.lines.size);

    for (i = 0; i < b.lines.size; ++i) {
        u32 j;
        Line cur_line = b.lines.data[i];

        for (j = cur_line.begin; j < cur_line.end; ++j) {
            putchar(b.data[j]);
        }
        putchar('\n');
    }

    buffer_kill(&b);
    return 0;
}
