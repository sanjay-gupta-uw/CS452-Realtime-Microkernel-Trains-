// #include <stddef.h>
#ifndef _util_h_
#define _util_h_

#include "rpi.h"
#include <cstdint> // For uint32_t

int a2d(char ch);
uint32_t a2ui(char **src, unsigned int base);
void ui2a(unsigned int num, unsigned int base, char *bf);
void i2a(int num, char *bf);

// format helper functions
void clear_to_end_line(size_t line);
void clear_screen(size_t line);
void move_cursor(size_t line, int row, int col);
void reset_formatting(size_t line);

// color helper functions
void color_black();
void color_red();
void color_green();
void color_yellow();
void color_blue();
void color_magenta();
void color_cyan();
void color_white();

#endif /* util.h */
