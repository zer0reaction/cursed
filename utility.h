#ifndef UTILITY_H_
#define UTILITY_H_

#include <stddef.h>
#include <stdbool.h>

#include "buffer.h"

size_t get_current_pos(Buffer *b);
void append_newline_maybe(Buffer *b);
char *line_next(char *start);
uint8_t char_size(char c);
uint32_t line_len(char *line);
uint32_t line_size(char *line);
uint32_t line_count(char *start);
char *line_goto(char *start, uint32_t n);
void adjust_col(Buffer *b);
void adjust_offset(Buffer *b, uint16_t screen_height);
void insert_substr(Buffer *b, size_t pos, char *str, uint32_t len);
void erase_substr(Buffer *b, size_t pos, uint32_t len);
bool is_sep(char c);
void data_resize(Buffer *b, size_t new_size);

#endif /* UTILITY_H_ */
