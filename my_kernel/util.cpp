#include "util.h"

// ascii digit to integer
int a2d(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

// ascii string to unsigned int, with base
char a2ui(char ch, char **src, unsigned int base, unsigned int *nump)
{
	unsigned int num;
	int digit;
	char *p;

	p = *src;
	num = 0;
	while ((digit = a2d(ch)) >= 0)
	{
		if ((unsigned int)digit > base)
			break;
		num = num * base + digit;
		ch = *p++;
	}
	*src = p;
	*nump = num;
	return ch;
}

// unsigned int to ascii string, with base
void ui2a(unsigned int num, unsigned int base, char *buf)
{
	unsigned int n = 0;
	unsigned int d = 1;

	while ((num / d) >= base)
		d *= base;
	while (d != 0)
	{
		unsigned int dgt = num / d;
		num %= d;
		d /= base;
		if (n || dgt > 0 || d == 0)
		{
			*buf++ = dgt + (dgt < 10 ? '0' : 'a' - 10);
			++n;
		}
	}
	*buf = 0;
}

// signed int to ascii string
void i2a(int num, char *buf)
{
	if (num < 0)
	{
		num = -num;
		*buf++ = '-';
	}
	ui2a(num, 10, buf);
}

/**********************************************************************
 * The following functions are for formatting text on the console.
 **********************************************************************/
void reset_formatting(size_t line)
{
	uart_puts(line, "\033[0m"); // Reset special formatting (such as colour).
}

// DISABLED FOR QEMU
void clear_screen(size_t line)
{
	return;
	// clear screen
	uart_puts(line, "\033[2J");
	// move cursor to top left
	uart_puts(line, "\033[H");
}

// DISABLED FOR QEMU
void clear_to_end_line(size_t line)
{
	return;
	uart_puts(line, "\033[K"); // wipe rest of line
}

void move_cursor(size_t line, int row, int col)
{
	// Send the escape sequence for cursor movement
	uart_putc(line, '\033'); // Escape character
	uart_putc(line, '[');	 // Start of control sequence

	// Send the row number
	if (row >= 10)
		uart_putc(line, '0' + (row / 10)); // Tens digit
	uart_putc(line, '0' + (row % 10));	  // Ones digit

	uart_putc(line, ';'); // Separator

	// Send the column number
	if (col >= 10)
		uart_putc(line, '0' + (col / 10)); // Tens digit
	uart_putc(line, '0' + (col % 10));	  // Ones digit

	uart_putc(line, 'H'); // End of sequence
}

void color_black() { uart_puts(CONSOLE, "\033[30m"); }
void color_red() { uart_puts(CONSOLE, "\033[31m"); }
void color_green() { uart_puts(CONSOLE, "\033[32m"); }
void color_yellow() { uart_puts(CONSOLE, "\033[33m"); }
void color_blue() { uart_puts(CONSOLE, "\033[34m"); }
void color_magenta() { uart_puts(CONSOLE, "\033[35m"); }
void color_cyan() { uart_puts(CONSOLE, "\033[36m"); }
void color_white() { uart_puts(CONSOLE, "\033[37m"); }