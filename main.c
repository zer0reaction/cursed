#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAB_SPACES 4

enum Buffer_Mode {
    NORMAL, INSERT
};
typedef enum Buffer_Mode Buffer_Mode;

typedef struct Buffer Buffer;
struct Buffer {
    char *data;
    char *path;
    bool saved;
    Buffer_Mode mode;
    size_t line, col;
    size_t line_off;
    size_t col_max;
};

/* TODO add header file */
char *line_goto(Buffer *b, size_t n);

/* ------------------------------------------------------------------------
Buffer functions
------------------------------------------------------------------------- */

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

void buf_save(Buffer *b) {
    FILE *fp = NULL;
    size_t len = 0;

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

void buf_append_newline_maybe(Buffer *b) {
    size_t len = 0;

    len = strlen(b->data);
    if (len == 0 || b->data[len - 1] != '\n') {
        b->data = realloc(b->data, len + 2);
        b->data[len] = '\n';
        b->data[len + 1] = '\0';
    }

    b->saved = false;
}

size_t buf_get_current_pos(Buffer *b) {
    char *ptr;

    ptr = line_goto(b, b->line) + b->col;
    return ptr - b->data;
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

size_t line_len(char *line) {
    size_t len = 0;

    while (line[len] != '\0' && line[len] != '\n') len++;
    return len;
}

size_t line_count(Buffer *b) {
    char *cur = NULL;
    size_t count = 0;

    cur = b->data;
    while ((cur = line_next(cur)) != NULL) count++;
    return count;
}

/* line number starts from 0 */
char *line_goto(Buffer *b, size_t n) {
    size_t i = 0;
    char *lp = NULL;

    lp = b->data;
    for (i = 0; i < n; ++i) {
        lp = line_next(lp);
        if (lp == NULL) return NULL;
    }
    return lp;
}

/* ------------------------------------------------------------------------
Move functions
------------------------------------------------------------------------- */

void move_down(Buffer *b) {
    if (b->line < line_count(b)) {
        char *lp = NULL;
        size_t len = 0;

        b->line++;

        lp = line_goto(b, b->line);
        len = line_len(lp);
        if (len < b->col_max) {
            b->col = len;
        } else {
            b->col = b->col_max;
        }
    }
}

void move_up(Buffer *b) {
    if (b->line > 0) {
        char *lp = NULL;
        size_t len = 0;

        b->line--;

        lp = line_goto(b, b->line);
        len = line_len(lp);
        if (len < b->col_max) {
            b->col = len;
        } else {
            b->col = b->col_max;
        }
    }
}

void move_right(Buffer *b) {
    char *lp = NULL;
    size_t len = 0;

    lp = line_goto(b, b->line);
    len = line_len(lp);
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

/* TODO rename */
void move_screen_down(Buffer *b, unsigned short int screen_height) {
    unsigned short int n = 0;
    unsigned long int pos = 0;

    n = screen_height / 2;
    pos = b->line + n;
    b->line = (pos > line_count(b)) ? line_count(b) : pos;
}

/* TODO rename */
void move_screen_up(Buffer *b, size_t screen_height) {
    unsigned short int n = 0;
    long int pos = 0;

    n = screen_height / 2;
    pos = b->line - n;
    b->line = (pos >= 0) ? pos : 0;
}

/* TODO rename */
void move_screen_center(Buffer *b, size_t screen_height) {
    long int off = 0;

    off = b->line - (screen_height / 2);
    b->line_off = (off >= 0) ? off : 0;
}

/* ------------------------------------------------------------------------
Edit functions
------------------------------------------------------------------------- */

void insert_char(Buffer *b, char c) {
    size_t i = 0;
    size_t size = 0;
    size_t pos = 0;

    if (b->line == line_count(b)) {
        buf_append_newline_maybe(b);
    }

    size = strlen(b->data) + 1;
    b->data = realloc(b->data, size + 1);

    pos = buf_get_current_pos(b);

    for (i = size; i > pos; --i) {
        b->data[i] = b->data[i - 1];
    }
    b->data[pos] = c;

    if (c == '\n') {
        b->line++;
        b->col_max = b->col = 0;
    } else {
        b->col++;
        b->col_max = b->col;
    }

    b->saved = false;
}

void delete_char(Buffer *b) {
    size_t i = 0;
    size_t len = 0;
    size_t pos = 0;
    size_t prev_line_len = 0;

    if (b->line == 0 && b->col == 0) return;

    if (b->line > 0) {
        prev_line_len = line_len(line_goto(b, b->line - 1));
    }

    pos = buf_get_current_pos(b);
    len = strlen(b->data);

    for (i = pos - 1; i < len - 1; ++i) {
        b->data[i] = b->data[i + 1];
    }
    b->data[len - 1] = '\0';

    if (b->col > 0) {
        b->col--;
        b->col_max = b->col;
    } else {
        b->col = b->col_max = prev_line_len;
        b->line--;
    }

    b->saved = false;
}

/* ------------------------------------------------------------------------
Render functions
------------------------------------------------------------------------- */

#define HEIGHT (LINES - 1)
#define WIDTH (COLS)

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
    char *lp = NULL;
    char status[128] = {0};

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    lp = line_goto(b, b->line_off);

    if (b->mode == INSERT) {
        sprintf(status, "insert, %s", b->path);
    } else if (b->mode == NORMAL) {
        sprintf(status, "normal, %s", b->path);
    }
    if (!(b->saved)) strcat(status, "*");

    erase();

    attron(COLOR_PAIR(1));
    mvaddstr(0, 0, status);
    attroff(COLOR_PAIR(1));

    mvaddstr(1, 0, lp);
    move(b->line - b->line_off + 1, b->col);
    refresh();
}

void render_offset_adjust(Buffer *b) {
    long int rel_line = 0;

    rel_line = b->line - b->line_off;

    if (rel_line < 0) {
        b->line_off += rel_line;
    } else if (rel_line > HEIGHT - 1) {
        b->line_off += rel_line - (HEIGHT - 1);
    }
}

int main(int argc, char **argv) {
    Buffer *b = NULL;
    bool should_close = false;

    if (argc != 2) return 1;
    b = buf_create_from_file(argv[1]);
    if (b == NULL) return 1;

    render_init();

    while (!should_close) {
        int c = 0;

        render_offset_adjust(b);
        render(b);
        c = getch();

        if (b->mode == NORMAL) {
            switch (c) {
                case 'q': should_close = true;           break;
                case 'i': b->mode = INSERT;              break;
                case 'j': move_down(b);                  break;
                case 'k': move_up(b);                    break;
                case 'l': move_right(b);                 break;
                case 'h': move_left(b);                  break;
                case 's': buf_save(b);                   break;
                case 'f': move_screen_center(b, HEIGHT); break;
                case 'n': {
                    move_screen_down(b, HEIGHT);
                    move_screen_center(b, HEIGHT);
                } break;
                case 'p': {
                    move_screen_up(b, HEIGHT);
                    move_screen_center(b, HEIGHT);
                } break;
            }
        /* only supporting ASCII for now */
        } else if (b->mode == INSERT && (c >= 0 && c <= 127)) {
            switch (c) {
                /* escape */
                case 27:
                    b->mode = NORMAL;
                    break;
                /* backspace */
                case 127:
                    delete_char(b);
                    break;
                /* tab */
                case 9: {
                    int i = 0;
                    for (i = 0; i < TAB_SPACES; ++i) insert_char(b, ' ');
                } break;
                default: insert_char(b, c);
            }
        }
    }

    render_end();
    buf_kill(b);
    return 0;
}
