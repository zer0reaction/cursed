#ifndef UTILITY_H_
#define UTILITY_H_

#include <stddef.h>

#include "buffer.h"

size_t get_current_pos(Buffer *b);
void append_newline_maybe(Buffer *b);
char *line_next(char *start);
unsigned char char_size(char c);
size_t line_len(char *line);
size_t line_size(char *line);
size_t line_count(char *start);
char *line_goto(char *start, size_t n);
void adjust_col(Buffer *b);
void adjust_offset(Buffer *b, size_t screen_height);
void insert_substr(Buffer *b, size_t pos, char *str, size_t len);
void erase_substr(Buffer *b, size_t pos, size_t len);

#endif /* UTILITY_H_ */
