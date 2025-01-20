#include <stddef.h>
#ifndef _util_h_
#define _util_h_ 1

#include "rpi.h"

int a2d(char ch);
char a2i(char ch, char **src, int base, int *nump);
void ui2a(unsigned int num, unsigned int base, char *bf);
void i2a(int num, char *bf);

// format helper functions
void clear_to_end_line(size_t line);
void clear_screen(size_t line);
void move_cursor(size_t line, int row, int col);
void reset_formatting(size_t line);

#endif /* util.h */
