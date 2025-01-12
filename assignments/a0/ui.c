#include "ui.h"
#include "rpi.h"
#include "clock.h"
// assume terminal is 80x24
#define NUM_ROWS 24
#define NUM_COLS 80

#define CLOCK_LENGTH 8

static void clear_screen()
{
   // clear screen
   uart_puts(CONSOLE, "\033[2J");
   // move cursor to top left
   uart_puts(CONSOLE, "\033[H");
}

static void move_cursor(int row, int col)
{
   char buf[12];
   snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
   uart_puts(CONSOLE, buf);
}

void draw_ui()
{

   clear_screen();

   // move cursor to top middle for clock
   move_cursor(1, (NUM_COLS - CLOCK_LENGTH) / 2);
   uart_puts(CONSOLE, "\033[36m"); // set colour to magenta
   clock_display();

   // Switches
   move_cursor(4, 1);
   uart_puts(CONSOLE, "\033[35m"); // set colour to cyan

   // table of switch positions
   uart_puts(CONSOLE, "Switches\r\n");
   uart_puts(CONSOLE, "--------\r\n");
   uart_puts(CONSOLE, "--------\r\n");
   uart_puts(CONSOLE, "--------\r\n");

   // Sensors
   move_cursor(10, 1);
   uart_puts(CONSOLE, "\033[33m"); // set colour to yellow
   // list of most recently triggered sensors
   uart_puts(CONSOLE, "Sensors\r\n");
   uart_puts(CONSOLE, "-------\r\n");
   uart_puts(CONSOLE, "-------\r\n");
   uart_puts(CONSOLE, "-------\r\n");

   move_cursor(18, 1);
   uart_puts(CONSOLE, "Press 'q' to reboot\r\n");

   move_cursor(16, 1);
   // set colour to green
   uart_puts(CONSOLE, "\033[32m");
   // command prompt for user input
   uart_puts(CONSOLE, "🤞🏾: ");
}