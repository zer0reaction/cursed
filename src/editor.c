#include <string.h>

#include "editor.h"
#include "config.h"
#include "buffer.h"
#include "utility.h"

extern char kill_buffer[KILL_BUFFER_SIZE];

void move_down(Buffer *b) {
    if (b->line < line_count(b->data) - 1) {
        b->line++;
        adjust_col(b);
    }
}

void move_up(Buffer *b) {
    if (b->line > 0) {
        b->line--;
        adjust_col(b);
    }
}

void move_right(Buffer *b) {
    char *lp = line_goto(b->data, b->line);
    int len = line_len(lp);
    if (b->col < len) {
        b->col++;
        b->col_max = b->col;
    }
}

void move_left(Buffer *b) {
    if (b->col > 0) {
        b->col--;
        b->col_max = b->col;
    }
}

// @refactor rename
void move_line_left(Buffer *b) {
    b->col = b->col_max = 0;
}

// @perf macro is not efficient
void move_forward(Buffer *b) {
    #define CHAR_AT_CUR(pos) (b->data[get_current_pos(b)])

    char c = CHAR_AT_CUR();

    if (c == '\0') return;

    if (is_sep(c)) {
        char rem = c;
        while (c != '\0' && c != '\n' && c == rem) {
            b->col++;
            b->col_max = b->col;
            c = CHAR_AT_CUR();
        }
        return;
    }

    while (c != '\0' && c != '\n' && !is_sep(c)) {
        b->col++;
        b->col_max = b->col;
        c = CHAR_AT_CUR();
    }

    #undef CHAR_AT_CUR
}

// @perf macro is not efficient
void move_backward(Buffer *b) {
    #define CHAR_BEFORE_CUR(void) (b->data[get_current_pos(b) - 1])

    if (b->line == 0 && b->col == 0) return;

    char c = CHAR_BEFORE_CUR();

    if (is_sep(c)) {
        char rem = c;
        while (b->col > 0 && c == rem) {
            b->col--;
            b->col_max = b->col;
            if (b->col > 0) c = CHAR_BEFORE_CUR();
        }
        return;
    }

    while (b->col > 0 && !is_sep(c)) {
        b->col--;
        b->col_max = b->col;
        if (b->col > 0) c = CHAR_BEFORE_CUR();
    }

    #undef CHAR_BEFORE_CUR
}

void move_line_begin(Buffer *b) {
    char *cur_line = line_goto(b->data, b->line);
    int len = line_len(cur_line);

    for (int i = 0; i < len; ++i) {
        if (cur_line[i] != ' ') {
            b->col = b->col_max = i;
            break;
        }
    }
}

void move_line_end(Buffer *b) {
    char *cur_line = line_goto(b->data, b->line);
    int len = line_len(cur_line);
    b->col = b->col_max = len;
}

// @refactor rename
void move_screen_down(Buffer *b, int screen_height) {
    int n = screen_height / 2;
    int pos = b->line + n;
    b->line = (pos > line_count(b->data) - 1) ? line_count(b->data) - 1: pos;
    adjust_col(b);
}

// @refactor rename
void move_screen_up(Buffer *b, int screen_height) {
    int n = screen_height / 2;
    int pos = b->line - n;
    b->line = (pos >= 0) ? pos : 0;
    adjust_col(b);
}

// @refactor rename
void move_screen_center(Buffer *b, int screen_height) {
    int off = b->line - (screen_height / 2);
    b->line_off = (off >= 0) ? off : 0;
    adjust_col(b);
}

void move_top(Buffer *b) {
    b->line = 0;
    adjust_col(b);
}

void move_bot(Buffer *b) {
    b->line = line_count(b->data) - 1;
    adjust_col(b);
}

void insert_char(Buffer *b, char c) {
    static int size = 0;
    static int acc = 0;
    static char buf[4] = {0};

    if (char_size(c) > 0) {
        size = char_size(c);
        acc = 0;
        memset(buf, 0, 4);
    }

    buf[acc++] = c;

    if (acc != 0 && acc == size) {
        size_t pos = get_current_pos(b);
        insert_substr(b, pos, buf, size);

        if (buf[0] == '\n') {
            b->col = b->col_max = 0;
            b->line++;
        } else {
            b->col++;
            b->col_max = b->col;
        }

        b->saved = false;
    }
}

void delete_char(Buffer *b) {
    size_t pos = 0;
    int prev_line_len = 0;

    if ((pos = get_current_pos(b)) == 0) return;
    pos--;

    if (b->line > 0) {
        char *prev_line = line_goto(b->data, b->line - 1);
        prev_line_len = line_len(prev_line);
    }

    while (char_size(b->data[pos]) == 0) pos--;

    erase_substr(b, pos, char_size(b->data[pos]));

    if (b->col > 0) {
        b->col--;
        b->col_max = b->col;
    } else {
        b->col = b->col_max = prev_line_len;
        b->line--;
    }

    b->saved = false;
}

void clear_killed(void) {
    memset(kill_buffer, 0, sizeof(kill_buffer));
}

void kill_line(Buffer *b) {
    char *cur_line = line_goto(b->data, b->line);
    int size = line_size(cur_line);
    int pos = cur_line - b->data;
    int killed = (cur_line[size] != '\0') ? size + 1 : size;

    strncat(kill_buffer, cur_line, killed);
    erase_substr(b, pos, killed);

    b->saved = false;
    adjust_col(b);
}

void clear_region(Buffer *b) {
    b->reg_begin = b->reg_end = NULL;
    b->reg_begin_line = b->reg_begin_col = 0;
}

void begin_region(Buffer *b) {
    char *cur = b->data + get_current_pos(b);
    b->reg_begin = b->reg_end = cur;
    b->reg_begin_line = b->line;
    b->reg_begin_col = b->col;
}

void end_region(Buffer *b) {
    char *cur = b->data + get_current_pos(b);

    if (cur >= b->reg_begin)
        b->reg_end = cur;
    else {
        char *tmp = b->reg_begin;
        b->reg_begin = cur;
        b->reg_end = tmp;

        b->reg_begin_line = b->line;
        b->reg_begin_col = b->col;
    }
}

void kill_region(Buffer *b) {
    if (b->reg_begin == b->reg_end) return;

    int reg_len = b->reg_end - b->reg_begin;

    strncat(kill_buffer, b->reg_begin, reg_len);
    erase_substr(b, b->reg_begin - b->data, reg_len);

    // @refactor maybe do this differently
    b->line = b->reg_begin_line;
    b->col = b->col_max = b->reg_begin_col;

    b->saved = false;
}

void copy_region(Buffer *b) {
    if (b->reg_begin == b->reg_end) return;

    int reg_len = b->reg_end - b->reg_begin;

    strncat(kill_buffer, b->reg_begin, reg_len);
}

void paste(Buffer *b) {
    int len = strlen(kill_buffer);
    size_t pos = get_current_pos(b);
    int count = line_count(kill_buffer);

    if (len == 0) return;

    insert_substr(b, pos, kill_buffer, len);

    b->line += count - 1;

    if (kill_buffer[len - 1] == '\n') {
        b->col = b->col_max = 0;
    } else if (count == 1) {
        b->col += line_len(kill_buffer);
        b->col_max = b->col;
    } else {
        char *lp = line_goto(kill_buffer, count - 1);
        b->col = b->col_max = line_len(lp);
    }

    b->saved = false;
}
