#ifndef UTILITY_H_
#define UTILITY_H_

#include <stdbool.h>

#include "buffer.h"

int get_current_pos(Buffer *b);
void append_newline_maybe(Buffer *b);
char *line_next(char *start);
int char_size(char c);
int line_len(char *line);
int line_size(char *line);
int line_count(char *start);
char *line_goto(char *start, int n);
void adjust_col(Buffer *b);
void adjust_offset(Buffer *b, int screen_height);
void insert_substr(Buffer *b, int pos, char *str, int len);
void erase_substr(Buffer *b, int pos, int len);
bool is_sep(char c);
void data_resize(Buffer *b, int new_size);

#endif // UTILITY_H_
