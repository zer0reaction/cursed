#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Buffer Buffer;
struct Buffer {
    char *data;
    size_t line;
    size_t col;
};

/* ------------------------------------------------------------------------
Buffer functions
------------------------------------------------------------------------- */

Buffer *buf_from_file(const char *path) {
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

    return b;
}

void buf_kill(Buffer *b) {
    free(b->data);
    free(b);
}

/* ------------------------------------------------------------------------
Line functions
------------------------------------------------------------------------- */

char *line_next(char *start) {
    char *cur = NULL;

    if (*start == '\0') return NULL;

    cur = start;
    while (*cur != '\n' && *cur != '\0') cur++;

    if (*cur == '\n') return cur + 1;
    else return cur;
}

size_t line_count(Buffer *b) {
    char *cur = NULL;
    size_t count = 0;

    cur = b->data;
    while ((cur = line_next(cur)) != NULL) count++;
    return count;
}

/* ------------------------------------------------------------------------
Move functions
------------------------------------------------------------------------- */

void move_down(Buffer *b) {
    if (b->line < line_count(b)) b->line++;
}

void move_up(Buffer *b) {
    if (b->line > 0) b->line--;
}

/* ------------------------------------------------------------------------
Render functions
------------------------------------------------------------------------- */

void render_init(void) {
    initscr();
    noecho();
    cbreak();
    nl();
}

void render_end(void) {
    endwin();
}

void render(Buffer *b) {
    refresh();
    mvaddstr(0, 0, b->data);
    move(b->line, b->col);
}

int main(int argc, char **argv) {
    Buffer *b = NULL;
    bool should_close = false;

    if (argc != 2) return 1;
    b = buf_from_file(argv[1]);

    render_init();

    while (!should_close) {
        int c = 0;

        render(b);

        c = getch();
        switch (c) {
            case 'q': should_close = true; break;
            case 'j': move_down(b); break;
            case 'k': move_up(b); break;
        }
    }

    render_end();
    buf_kill(b);
    return 0;
}
