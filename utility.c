#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "utility.h"

static char seps[] = {
    '!', '"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.',
    '/', ':', ';', '<', '=', '>', '?',
    '@', '[', '\\', ']', '^', '_', '`',
    '{', '|', '}', '~', ' '
};

size_t get_current_pos(Buffer *b) {
    size_t i = 0;
    char *ptr = NULL;

    ptr = line_goto(b->data, b->line);
    for (i = 0; i < b->col; ++i) {
        ptr += char_size(*ptr);
    }

    return ptr - b->data;
}

void append_newline_maybe(Buffer *b) {
    size_t len = 0;

    len = strlen(b->data);
    if (len == 0 || b->data[len - 1] != '\n') {
        b->data = realloc(b->data, len + 2);
        b->data[len] = '\n';
        b->data[len + 1] = '\0';
        b->saved = false;
    }
}

char *line_next(char *start) {
    char *cur = NULL;

    if (*start == '\0') return NULL;

    cur = start;
    while (*cur != '\n' && *cur != '\0') cur++;

    if (*cur == '\n') return cur + 1;
    else return cur;
}

unsigned char char_size(char c) {
    if      ((c & 0x80) == 0x00) return 1;
    else if ((c & 0xC0) == 0x80) return 0;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    else return 0;
}

size_t line_len(char *line) {
    size_t len = 0;
    size_t pos = 0;

    while (line[pos] != '\0' && line[pos] != '\n') {
        unsigned char csize = char_size(line[pos]);

        pos += csize;
        len++;
    }

    return len;
}

size_t line_size(char *line) {
    size_t size = 0;

    while (line[size] != '\0' && line[size] != '\n') size++;
    return size;
}

size_t line_count(char *start) {
    size_t count = 0;

    while (*start != '\0') {
        if (*start == '\n') count++;
        start++;
    }

    return count + 1;
}

/* line number starts from 0 */
char *line_goto(char *start, size_t n) {
    size_t i = 0;

    for (i = 0; i < n; ++i) {
        start = line_next(start);
        if (start == NULL) return NULL;
    }
    return start;
}

void adjust_col(Buffer *b) {
    char *lp = NULL;
    size_t len = 0;

    lp = line_goto(b->data, b->line);
    len = line_len(lp);
    if (len < b->col_max) {
        b->col = len;
    } else {
        b->col = b->col_max;
    }
}

void adjust_offset(Buffer *b, size_t screen_height) {
    long int rel_line = 0;

    rel_line = b->line - b->line_off;

    if (rel_line < 0) {
        b->line_off += rel_line;
    } else if (rel_line > (int)screen_height - 1) {
        b->line_off += rel_line - (screen_height - 1);
    }
}

void insert_substr(Buffer *b, size_t pos, char *str, size_t len) {
    size_t i = 0;
    size_t buf_len = 0;

    buf_len = strlen(b->data);

    b->data = realloc(b->data, buf_len + len + 1);
    b->data[buf_len + len] = '\0';

    for (i = buf_len + len - 1; i >= pos + len; --i) {
        b->data[i] = b->data[i - len];
    }

    strncpy(b->data + pos, str, len);
}

void erase_substr(Buffer *b, size_t pos, size_t len) {
    size_t i = 0;
    size_t buf_len = 0;

    buf_len = strlen(b->data);

    for (i = pos; i < buf_len - len; ++i) {
        b->data[i] = b->data[i + len];
    }
    b->data[buf_len - len] = '\0';

    b->data = realloc(b->data, buf_len - len + 1);
}

bool is_sep(char c) {
    size_t i = 0;

    for (i = 0; i < sizeof(seps); ++i) {
        if (c == seps[i]) return true;
    }

    return false;
}
