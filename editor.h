#ifndef EDITOR_H_
#define EDITOR_H_

#include "buffer.h"

void move_down(Buffer *b);
void move_up(Buffer *b);
void move_right(Buffer *b);
void move_left(Buffer *b);
void move_line_left(Buffer *b);
void move_forward(Buffer *b);
void move_backward(Buffer *b);
void move_line_begin(Buffer *b);
void move_line_end(Buffer *b);
void move_screen_down(Buffer *b, int screen_height);
void move_screen_up(Buffer *b, int screen_height);
void move_screen_center(Buffer *b, int screen_height);
void insert_char(Buffer *b, char c);
void delete_char(Buffer *b);
void clear_killed(void);
void kill_line(Buffer *b);
void clear_region(Buffer *b);
void begin_region(Buffer *b);
void end_region(Buffer *b);
void kill_region(Buffer *b);
void copy_region(Buffer *b);
void paste(Buffer *b);

#endif // EDITOR_H_
