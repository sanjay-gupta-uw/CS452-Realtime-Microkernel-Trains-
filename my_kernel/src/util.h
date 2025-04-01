// #include <stddef.h>
#ifndef _util_h_
#define _util_h_

#include "rpi.h"
#include <cstdint> // For uint32_t

int a2d(char ch);
uint32_t a2ui(char **src, unsigned int base);
void ui2a(unsigned int num, unsigned int base, char *bf);
void i2a(int num, char *bf);
int get_index_from_name(const char *name);
#endif /* util.h */
