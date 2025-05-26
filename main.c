#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DA_INIT_CAP 128

#define KEY_ESCAPE 27

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define da_resize(da, new_size)                                                 \
do {                                                                            \
    assert((da)->size <= (da)->capacity);                                       \
                                                                                \
    if ((da)->capacity == 0) {                                                  \
        (da)->capacity = MAX(DA_INIT_CAP, (new_size) * 2);                      \
        (da)->items = realloc((da)->items,                                      \
                              sizeof(*(da)->items) * (da)->capacity);           \
        assert((da)->items && "Buy more RAM?");                                 \
    } else if ((new_size) > (da)->capacity) {                                   \
        (da)->capacity = (new_size) * 2;                                        \
        (da)->items = realloc((da)->items,                                      \
                              sizeof(*(da)->items) * (da)->capacity);           \
        assert((da)->items && "Buy more RAM?");                                 \
    } else if ((new_size) <= (da)->capacity / 4 && (new_size) >= DA_INIT_CAP) { \
        (da)->capacity = (new_size) * 2;                                        \
        (da)->items = realloc((da)->items,                                      \
                              sizeof(*(da)->items) * (da)->capacity);           \
        assert((da)->items && "Buy more RAM?");                                 \
    }                                                                           \
                                                                                \
    (da)->size = (new_size);                                                    \
} while (0)

#define da_append(da, item)                                                \
do {                                                                       \
    da_resize((da), (da)->size + 1);                                       \
    (da)->items[(da)->size - 1] = (item);                                  \
} while (0)

#define lines_append(lines, line) da_append(lines, line)
#define sb_resize(sb, new_size) da_resize(sb, new_size)

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

typedef enum Mode {
    NORMAL_MODE = 0,
    INSERT_MODE = 1
} Mode;

typedef struct Buffer {
    String_Builder data;
    Lines lines;
    Mode mode;

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

s8 char_size(char c)
{
    if      ((c & 0x80) == 0x00) return 1;
    else if ((c & 0xC0) == 0x80) return 0;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    else return -1;
}

/* line functions */

void lines_retokenize(Lines *lines, String_Builder *sb)
{
    u32 i = 0;
    Line line = {0};

    lines->size = 0;

    for (i = 0; i < sb->size; ++i) {
        if (sb->items[i] == '\n') {
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

    sb_resize(&b->data, size);
    fread(b->data.items, 1, size, fp);

    lines_retokenize(&b->lines, &b->data);

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

void move_top(Buffer *b)
{
    b->cursor = 0;
    update_last_col(b);
    adjust_row_offset(b);
}

void move_bot(Buffer *b)
{
    assert(b->lines.size > 0);

    {
        Line bottom_line = b->lines.items[b->lines.size - 1];
        b->cursor = bottom_line.begin;
        update_last_col(b);
        adjust_row_offset(b);
    }
}

void insert_char_at_cursor(Buffer *b, char c)
{
    static u8 size = 0;
    static u8 accum = 0;
    static char buf[4] = {0};

    assert(char_size(c) != -1);

    if (char_size(c) > 0) {
        size = char_size(c);
        accum = 0;
        memset(buf, 0, 4);
    }

    buf[accum] = c;
    accum++;

    if (accum == size && accum != 0) {
        sb_resize(&b->data, b->data.size + size);
        memmove(&b->data.items[b->cursor + size],
                &b->data.items[b->cursor],
                b->data.size - b->cursor);
        memcpy(&b->data.items[b->cursor], buf, size);

        b->cursor += size;
        lines_retokenize(&b->lines, &b->data);
        update_last_col(b);

        if (buf[0] == '\n') {
            adjust_row_offset(b);
        }
    }
}

/* render functions */

void render(Buffer *b)
{
    u32 i, j;
    u32 cursor_row;
    u32 cursor_col;
    Line cursor_line;

    curs_set(0);
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
    curs_set(1);
}

bool process_movement(Buffer *b, int c)
{
    bool moved = true;

    switch (c) {
    case 'j':
        move_down(b);
        break;
    case 'k':
        move_up(b);
        break;
    case 'h':
        move_left(b);
        break;
    case 'l':
        move_right(b);
        break;
    case 'g':
        move_top(b);
        break;
    case 'G':
        move_bot(b);
        break;
    default:
        moved = false;
    }

    return moved;
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

        if (b.mode == NORMAL_MODE) {
            if (process_movement(&b, c)) continue;

            switch (c) {
            case 'q':
                should_close = true;
                break;
            case 'i':
                b.mode = INSERT_MODE;
                break;
            }
        } else if (b.mode == INSERT_MODE) {
            switch (c) {
            case KEY_ESCAPE:
                b.mode = NORMAL_MODE;
                break;
            default:
                insert_char_at_cursor(&b, c);
                break;
            }
        }
    }

    endwin();
    buffer_kill(&b);
    return 0;
}
