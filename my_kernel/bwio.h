#ifndef KERN_LIB_BWIO_H_
#define KERN_LIB_BWIO_H_

#include "ts7200.h"

typedef const char *va_list;

#define __va_argsiz(t) \
   (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list)__builtin_next_arg(pN)))

#define va_end(ap) ((void)0)

#define va_arg(ap, t) \
   (((ap) = (ap) + __va_argsiz(t)), *((t *)(void *)((ap) - __va_argsiz(t))))

#define ON 1
#define OFF 0

int bwsetfifo(unsigned int channel, int state);

int bwsetspeed(unsigned int channel, int speed);

int bwsetstp2(unsigned int channel, int select);

int bwputc(unsigned int channel, char c);

int bwgetc(unsigned int channel);

int bwputx(unsigned int channel, char c);

int bwputstr(unsigned int channel, const char *str);

extern "C" int bwputr(unsigned int channel, unsigned int reg);

void bwputw(unsigned int channel, int n, char fc, const char *bf);

void bwprintf(unsigned int channel, const char *format, ...);

#endif // KERN_LIB_BWIO_H_