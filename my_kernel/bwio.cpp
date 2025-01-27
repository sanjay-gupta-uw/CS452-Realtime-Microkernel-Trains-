/*
 * bwio.c - busy-wait I/O routines for diagnosis
 *
 * Specific to the TS-7200 ARM evaluation board
 *
 */

#include "bwio.h"

/*
 * The UARTs are initialized by RedBoot to the following state
 * 	115,200 bps
 * 	8 bits
 * 	no parity
 * 	fifos enabled
 */
int bwsetfifo(unsigned int channel, int state)
{
   volatile int *line = (int *)(channel + UART_LCRH_OFFSET);
   int buf = *line;
   buf = state ? buf | FEN_MASK : buf & ~FEN_MASK;
   *line = buf;
   return 0;
}

int bwsetspeed(unsigned int channel, int speed)
{
   volatile int *mid, *low;
   int baudDiv;
   mid = (int *)(channel + UART_LCRM_OFFSET);
   low = (int *)(channel + UART_LCRL_OFFSET);
   baudDiv = UARTCLK / (16 * speed) - 1;
   if (0 < baudDiv && baudDiv <= 0xffff)
   {
      *mid = (baudDiv >> 8) & 0xff;
      *low = baudDiv & 0xff;
      return 0;
   }
   return -1;
}

int bwsetstp2(unsigned int channel, int select)
{
   unsigned int base = channel;
   int *high, val;
   high = (int *)(base + UART_LCRH_OFFSET);
   val = *high;
   val = select ? val | STP2_MASK : val & ~STP2_MASK;
   *high = val;
   return 0;
}

int bwputc(unsigned int channel, char c)
{
   volatile int *flags, *data;
   flags = (int *)(channel + UART_FLAG_OFFSET);
   data = (int *)(channel + UART_DATA_OFFSET);

   while ((*flags & TXFF_MASK))
      ;
   *data = c;
   while (!(*flags & TXFE_MASK))
      ;

   return 0;
}

char bwc2x(char ch)
{
   if ((ch <= 9))
      return '0' + ch;
   return 'a' + ch - 10;
}

int bwputx(unsigned int channel, char c)
{
   char chh, chl;

   chh = bwc2x(c / 16);
   chl = bwc2x(c % 16);
   bwputc(channel, chh);
   return bwputc(channel, chl);
}

int bwputr(unsigned int channel, unsigned int reg)
{
   int byte;
   char *ch = (char *)&reg;

   for (byte = 3; byte >= 0; byte--)
      bwputx(channel, ch[byte]);
   return bwputc(channel, ' ');
}

int bwputstr(unsigned int channel, const char *str)
{
   while (*str)
   {
      if (bwputc(channel, *str) < 0)
         return -1;
      str++;
   }
   return 0;
}

void bwputw(unsigned int channel, int n, char fc, const char *bf)
{
   char ch;
   const char *p = bf;

   while (*p++ && n > 0)
      n--;
   while (n-- > 0)
      bwputc(channel, fc);
   while ((ch = *bf++))
      bwputc(channel, ch);
}

int bwgetc(unsigned int channel)
{
   volatile int *flags, *data;
   flags = (int *)(channel + UART_FLAG_OFFSET);
   data = (int *)(channel + UART_DATA_OFFSET);

   unsigned char c;

   while ((*flags & RXFE_MASK))
      ;
   c = *data;
   return c;
}

int bwa2d(char ch)
{
   if (ch >= '0' && ch <= '9')
      return ch - '0';
   if (ch >= 'a' && ch <= 'f')
      return ch - 'a' + 10;
   if (ch >= 'A' && ch <= 'F')
      return ch - 'A' + 10;
   return -1;
}

char bwa2i(char ch, const char **src, int base, int *nump)
{
   int num, digit;
   const char *p;

   p = *src;
   num = 0;
   while ((digit = bwa2d(ch)) >= 0)
   {
      if (digit > base)
         break;
      num = num * base + digit;
      ch = *p++;
   }
   *src = p;
   *nump = num;
   return ch;
}

void bwui2a(unsigned int num, unsigned int base, char *bf)
{
   int n = 0;
   int dgt;
   unsigned int d = 1;

   while ((num / d) >= base)
      d *= base;
   while (d != 0)
   {
      dgt = num / d;
      num %= d;
      d /= base;
      if (n || dgt > 0 || d == 0)
      {
         *bf++ = dgt + (dgt < 10 ? '0' : 'a' - 10);
         ++n;
      }
   }
   *bf = 0;
}

void bwi2a(int num, char *bf)
{
   if (num < 0)
   {
      num = -num;
      *bf++ = '-';
   }
   bwui2a(num, 10, bf);
}

void bwformat(unsigned int channel, const char *fmt, va_list va)
{
   char bf[12];
   char ch, lz;
   int w;

   while ((ch = *(fmt++)))
   {
      if (ch != '%')
         bwputc(channel, ch);
      else
      {
         lz = 0;
         w = 0;
         ch = *(fmt++);
         switch (ch)
         {
         case '0':
            lz = '0';
            ch = *(fmt++);
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            if (!lz)
               lz = ' ';
            ch = bwa2i(ch, &fmt, 10, &w);
            break;
         }
         switch (ch)
         {
         case 0:
            return;
         case 'c':
            bwputc(channel, va_arg(va, char));
            break;
         case 's':
            bwputw(channel, w, 0, va_arg(va, char *));
            break;
         case 'u':
            bwui2a(va_arg(va, unsigned int), 10, bf);
            bwputw(channel, w, lz, bf);
            break;
         case 'd':
            bwi2a(va_arg(va, int), bf);
            bwputw(channel, w, lz, bf);
            break;
         case 'x':
            bwui2a(va_arg(va, unsigned int), 16, bf);
            bwputw(channel, w, lz, bf);
            break;
         case '%':
            bwputc(channel, ch);
            break;
         }
      }
   }
}

void bwprintf(unsigned int channel, const char *fmt, ...)
{
   va_list va;

   va_start(va, fmt);
   bwformat(channel, fmt, va);
   va_end(va);
}