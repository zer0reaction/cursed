#include <ncurses.h>
#include <stdbool.h>
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
    u32 row_offset;
    u32 last_col;
} Buffer;

/* utility functions */

u32 get_cursor_row(Buffer *b)
{
    u32 i;

    assert(b->lines.size > 0);

    for (i = 0; i < b->lines.size; ++i) {
        Line line = b->lines.items[i];

        if (b->cursor >= line.begin && b->cursor <= line.end) {
            return i;
        }
    }

    return b->lines.size - 1;
}

void adjust_row_offset(Buffer *b)
{
    u32 cursor_row = get_cursor_row(b);
    s32 relative_row = (s32)cursor_row - (s32)b->row_offset;

    if (relative_row < 0) {
        b->row_offset += relative_row;
    } else if (relative_row > LINES - 1) {
        b->row_offset += relative_row - (LINES - 1);
    }
}

void update_last_col(Buffer *b)
{
    u32 cursor_row = get_cursor_row(b);
    Line cursor_line = b->lines.items[cursor_row];
    b->last_col = b->cursor - cursor_line.begin;
}

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

/* editor functions */

void move_down(Buffer *b)
{
    u32 cursor_row = get_cursor_row(b);

    if (cursor_row < b->lines.size - 1) {
        Line next_line = b->lines.items[cursor_row + 1];
        u32 next_line_len = next_line.end - next_line.begin;
        u32 cursor_col;

        if (next_line_len < b->last_col) {
            cursor_col = next_line_len;
        } else {
            cursor_col = b->last_col;
        }

        b->cursor = next_line.begin + cursor_col;
        adjust_row_offset(b);
    }
}

void move_up(Buffer *b)
{
    u32 cursor_row = get_cursor_row(b);

    if (cursor_row > 0) {
        Line prev_line = b->lines.items[cursor_row - 1];
        u32 prev_line_len = prev_line.end - prev_line.begin;
        u32 cursor_col;

        if (prev_line_len < b->last_col) {
            cursor_col = prev_line_len;
        } else {
            cursor_col = b->last_col;
        }

        b->cursor = prev_line.begin + cursor_col;
        adjust_row_offset(b);
    }
}

void move_right(Buffer *b)
{
    if (b->cursor < b->data.size) {
        b->cursor++;
        update_last_col(b);
        adjust_row_offset(b);
    }
}

void move_left(Buffer *b)
{
    if (b->cursor > 0) {
        b->cursor--;
        update_last_col(b);
        adjust_row_offset(b);
    }
}

/* render functions */

void render(Buffer *b)
{
    u32 i, j;
    u32 cursor_row;
    u32 cursor_col;
    Line cursor_line;

    erase();

    for (i = 0; i < (u32)LINES; ++i) {
        if (i + b->row_offset >= b->lines.size) break;

        {
            Line line = b->lines.items[i + b->row_offset];

            for (j = 0; j < line.end - line.begin; ++j) {
                mvaddch(i, j, b->data.items[line.begin + j]);
            }
        }
    }

    refresh();

    cursor_row = get_cursor_row(b);
    cursor_line = b->lines.items[cursor_row];
    cursor_col = b->cursor - cursor_line.begin;

    move(cursor_row - b->row_offset, cursor_col);
}

int main(int argc, char **argv)
{
    bool should_close = false;
    Buffer b = {0};

    if (argc != 2) return 1;

    buffer_from_file(&b, argv[1]);
    if (!b.lines.size) return 1;

    initscr();
    noecho();
    nl();
    cbreak();

    while (!should_close) {
        int c;

        render(&b);

        c = getch();
        switch (c) {
        case 'q':
            should_close = true;
            break;
        case 'j':
            move_down(&b);
            break;
        case 'k':
            move_up(&b);
            break;
        case 'h':
            move_left(&b);
            break;
        case 'l':
            move_right(&b);
            break;
        }
    }

    endwin();
    buffer_kill(&b);
    return 0;
}
