#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <assert.h>

// @cleanup find the header
int mvaddwstr(int y, int x, const wchar_t *wstr);

#include "config.h"
#include "utility.h"
#include "buffer.h"
#include "editor.h"

char kill_buffer[KILL_BUFFER_SIZE] = {0};

#define HEIGHT (LINES - 1)
#define WIDTH (COLS)

// @feat make this better
void render(Buffer *b) {
    curs_set(0);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    wchar_t wcbuf[10 * 1024] = {0};
    char *lp = line_goto(b->data, b->line_off);
    mbstowcs(wcbuf, lp, 10 * 1024);

    char status[128] = {0};

    if (b->mode == INSERT_MODE) {
        sprintf(status, "[insert] %s", b->path);
    } else if (b->mode == REGION_MODE) {
        sprintf(status, "[region] %s", b->path);
    } else if (b->mode == NORMAL_MODE) {
        sprintf(status, "%s", b->path);
    }
    if (!(b->saved)) strcat(status, "*");
    sprintf(status + strlen(status), ":%u:%u (%lu)",
            b->line + 1, b->col + 1, strlen(kill_buffer));

    erase();

    attron(COLOR_PAIR(1));
    mvaddstr(0, 0, status);
    attroff(COLOR_PAIR(1));

    mvaddwstr(1, 0, wcbuf);
    move(b->line - b->line_off + 1, b->col);
    refresh();

    curs_set(1);
}

int main(int argc, char **argv) {
    if (argc == 1 || argc > 5) return 1;

    setlocale(LC_ALL, "");

    // @feat add support for more than 4 buffers
    Buffer *b = NULL;
    Buffer *buf_list[4] = {0};

    for (int i = 0; i < argc - 1; ++i) {
        buf_list[i] = buf_open(argv[i + 1]);
    }
    b = buf_list[0];

    initscr();
    noecho();
    cbreak();
    nl();

    bool should_close = false;

    while (!should_close) {
        // @refactor this should be in moves?
        adjust_offset(b, HEIGHT);

        render(b);
        int c = getch();

// @refactor this macro isn't the best idea
#define SWITCH_MOVE \
        case 'j': \
            move_down(b); \
            break; \
        case 'k': \
            move_up(b); \
            break; \
        case 'l': \
            move_right(b); \
            break; \
        case 'h': \
            move_left(b); \
            break; \
        case 'f': \
            move_forward(b); \
            break; \
        case 'b': \
            move_backward(b); \
            break; \
        case '^': \
            move_line_begin(b); \
            break; \
        case '$': \
            move_line_end(b); \
            break; \
        case '0': \
            move_line_left(b); \
            break; \
        case 'e': \
            move_screen_center(b, HEIGHT); \
            break; \
        case 'n': \
            move_screen_down(b, HEIGHT); \
            move_screen_center(b, HEIGHT); \
            break; \
        case 'p': \
            move_screen_up(b, HEIGHT); \
            move_screen_center(b, HEIGHT); \
            break; \
        case 'g': \
            move_top(b); \
            break; \
        case 'G': \
            move_bot(b); \
            move_screen_center(b, HEIGHT); \
            break;

        if (b->mode == NORMAL_MODE) {
            switch (c) {
                SWITCH_MOVE
                case 'q':
                    // @feat check for all buffers to be saved
                    //       before closing editor
                    if (b->saved) should_close = true;
                    break;
                case 'Q':
                    should_close = true;
                    break;
                case 'i':
                    b->mode = INSERT_MODE;
                    break;
                case 'a':
                    move_right(b);
                    b->mode = INSERT_MODE;
                    break;
                case 'I':
                    move_line_begin(b);
                    b->mode = INSERT_MODE;
                    break;
                case 'A':
                    move_line_end(b);
                    b->mode = INSERT_MODE;
                    break;
                case 's':
                    buf_save(b);
                    break;
                case 'd':
                    kill_line(b);
                    break;
                case 'r':
                    clear_killed();
                    break;
                case 'y':
                    paste(b);
                    break;
                case ' ':
                    begin_region(b);
                    b->mode = REGION_MODE;
                    break;
                case 'o':
                    move_line_end(b);
                    insert_char(b, '\n');
                    b->mode = INSERT_MODE;
                    break;
                case 'O':
                    move_line_left(b);
                    insert_char(b, '\n');
                    move_up(b);
                    b->mode = INSERT_MODE;
                    break;
                // Ctrl+j
                case 10:
                    b = (buf_list[0] != NULL) ? buf_list[0] : b;
                    break;
                // Ctrl+k
                case 11:
                    b = (buf_list[1] != NULL) ? buf_list[1] : b;
                    break;
                // Ctrl+l
                case 12:
                    b = (buf_list[2] != NULL) ? buf_list[2] : b;
                    break;
                // Ctrl+;
                case 59:
                    b = (buf_list[3] != NULL) ? buf_list[3] : b;
                    break;
            }
        } else if (b->mode == INSERT_MODE) {
            switch (c) {
                // escape
                case 27:
                    b->mode = NORMAL_MODE;
                    break;
                // backspace
                case 127:
                case 8:
                    delete_char(b);
                    break;
                // tab
                case 9: {
                    for (int i = 0; i < TAB_SPACES; ++i) {
                        insert_char(b, ' ');
                    }
                } break;
                default: insert_char(b, c);
            }
        } else if (b->mode == REGION_MODE) {
            switch (c) {
                SWITCH_MOVE
                // escape
                case 27:
                case ' ':
                    clear_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case 'd':
                    end_region(b);
                    kill_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case 'c':
                    end_region(b);
                    copy_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case 'r':
                    clear_killed();
                    break;
                // Ctrl+j
                case 10:
                    b = (buf_list[0] != NULL) ? buf_list[0] : b;
                    break;
                // Ctrl+k
                case 11:
                    b = (buf_list[1] != NULL) ? buf_list[1] : b;
                    break;
                // Ctrl+l
                case 12:
                    b = (buf_list[2] != NULL) ? buf_list[2] : b;
                    break;
                // Ctrl+;
                case 59:
                    b = (buf_list[3] != NULL) ? buf_list[3] : b;
                    break;
            }
        }
#undef SWITCH_MOVE
    }

    endwin();

    for (int i = 0; i < argc - 1; ++i) {
        buf_kill(buf_list[i]);
    }

    return 0;
}
