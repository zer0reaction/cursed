#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define da_append(da, item)                                             \
do {                                                                    \
    assert((da)->size <= (da)->capacity);                               \
                                                                        \
    if ((da)->items == NULL) {                                          \
        (da)->items = malloc(sizeof(*((da)->items)) * 1);               \
        (da)->size = 0;                                                 \
        (da)->capacity = 1;                                             \
    } else if ((da)->size >= (da)->capacity) {                          \
        (da)->capacity *= 2;                                            \
        (da)->items = realloc((da)->items,                              \
                              sizeof(*((da)->items)) * (da)->capacity); \
    }                                                                   \
                                                                        \
    (da)->items[(da)->size] = (item);                                   \
    (da)->size++;                                                       \
} while (0)

#define da_append_many(da, items, n)                                    \
do {                                                                    \
    u32 i;                                                              \
                                                                        \
    assert((da)->size <= (da)->capacity);                               \
                                                                        \
    if ((da)->items == NULL) {                                          \
        (da)->items = malloc(sizeof(*((da)->items)) * (n));             \
        (da)->size = 0;                                                 \
        (da)->capacity = (n);                                           \
    } else if ((da)->size + (n) >= (da)->capacity) {                    \
        (da)->capacity = ((da)->size + (n)) * 2;                        \
        (da)->items = realloc((da)->items,                              \
                              sizeof(*((da)->items)) * (da)->capacity); \
    }                                                                   \
                                                                        \
    for (i = 0; i < (n); ++i) {                                         \
        (da)->items[(da)->size + i] = (items)[i];                       \
    }                                                                   \
} while (0)

#define lines_append(lines, line) da_append(lines, line)

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
    Line *items;
    u32 size;
    u32 capacity;
} Lines;

typedef struct String_Builder {
    char *items;
    u32 size;
    u32 capacity;
} String_Builder;

typedef struct Buffer {
    String_Builder data;
    Lines lines;

    u32 cursor;
} Buffer;

/* line functions */

void lines_retokenize(Lines *lines, const char *data, u32 len)
{
    u32 i = 0;
    Line line = {0};

    lines->size = 0;

    for (i = 0; i < len; ++i) {
        if (data[i] == '\n') {
            line.end = i;
            lines_append(lines, line);

            line.begin = i + 1;
            line.end = 0;
        }
    }
    line.end = i;
    lines_append(lines, line);
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

    memset(b, 0, sizeof(Buffer));

    b->data.items = malloc(size);
    b->data.size = size;
    b->data.capacity = size;
    fread(b->data.items, 1, size, fp);

    lines_retokenize(&(b->lines), b->data.items, b->data.size);

    fclose(fp);
    return b->lines.size;
}

void buffer_kill(Buffer *b)
{
    free(b->data.items);
    free(b->lines.items);
}

int main(int argc, char **argv)
{
    u32 i;
    Buffer b = {0};

    if (argc != 2) return 1;

    buffer_from_file(&b, argv[1]);
    if (!b.lines.size) return 1;
    printf("%u line(s) loaded\n", b.lines.size);

    printf("data size: %u, data capacity: %u\n", b.data.size, b.data.capacity);
    printf("lines size: %u, lines capacity: %u\n", b.lines.size, b.lines.capacity);

    buffer_kill(&b);
    return 0;
}
