#include "../include/io.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include <stdarg.h>
#include "../../util.h"
// #include "../../rpi.h"

#define CONSOLE 1
#define MARKLIN 2

IO::IO()
{
    IO_SERVER_TID = WHOIS("IOServer");
    if (IO_SERVER_TID < 0)
    {
        // uart_printf(CONSOLE, "Error finding IO Server\r\n");
        // EXIT();
    }
}

IO::~IO()
{
}

// #include <stdint.h>
// Helper function to handle padding
void IO::pad_and_print(const char *str, int width)
{
    int len = 0;
    const char *temp = str;

    // Calculate string length
    while (*temp++)
        len++;

    // Add padding spaces if necessary
    while (len < width)
    {
        int retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, ' ');
        width--;
    }

    Print(str);
}

void IO::Print(const char *fmt, ...)
{
    // return; // disable uart_printf
    va_list va;
    char ch, buf[12];
    int width = 0; // Width for padding
    int retval = -1;

    va_start(va, fmt);
    while ((ch = *(fmt++)))
    {
        if (ch != '%')
        {
            // DEPRECATE CHANNEL SINCE INDIVIDUAL IO SERVERS
            retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, ch);
            // uart_printf(CONSOLE, "retval: %d\n", retval);
        }
        else
        {
            // Parse padding width if present
            width = 0;
            ch = *(fmt++);
            if (ch >= '0' && ch <= '9')
            {
                while (ch >= '0' && ch <= '9') // Extract width
                {
                    width = width * 10 + (ch - '0');
                    ch = *(fmt++);
                }
            }

            switch (ch)
            {
            case 'u': // Unsigned integer
                ui2a(va_arg(va, unsigned int), 10, buf);
                pad_and_print(buf, width);
                break;

            case 'd': // Signed integer
                i2a(va_arg(va, int), buf);
                pad_and_print(buf, width);
                break;

            case 'x': // Hexadecimal
                ui2a(va_arg(va, unsigned int), 16, buf);
                pad_and_print(buf, width);
                break;

            case 's': // String
            {
                char *str = va_arg(va, char *);
                pad_and_print(str, width);
                break;
            }

            case 'c':                       // Character
                ch = (char)va_arg(va, int); // Characters are promoted to int in varargs
                if (width > 1)
                {
                    for (int i = 1; i < width; ++i) // Add padding spaces
                    {
                        retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, ' ');
                    }
                }
                retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, ch); // Print the character
                break;

            case '%': // Literal '%'
                retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, '%');
                break;

            case '\0': // End of format string
                return;

            default:
                // Handle unknown specifier by printing it
                retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, '%');
                // uart_putc(line, '%');
                retval = IO_SERVER::Putc(IO_SERVER_TID, CONSOLE, ch);
                break;
            }
        }
    }
    va_end(va);
}

void IO::reset_formatting()
{
    Print("\033[0m");
}

// DISABLED FOR QEMU
void IO::clear_screen()
{
    // clear screen
    Print("\033[2J");
    // move cursor to top left
    Print("\033[H");
}

// DISABLED FOR QEMU
void IO::clear_to_end_line()
{
    Print("\033[K"); // wipe rest of line
}

void IO::move_cursor(int row, int col)
{
    Print("\033[%d;%dH", row, col);
}

void IO::SaveCursor()
{
    Print("\033[s");
}

void IO::RestoreCursor()
{
    Print("\033[u");
}

void IO::color_black() { Print("\033[30m"); }
void IO::color_red() { Print("\033[31m"); }
void IO::color_green() { Print("\033[32m"); }
void IO::color_yellow() { Print("\033[33m"); }
void IO::color_blue() { Print("\033[34m"); }
void IO::color_magenta() { Print("\033[35m"); }
void IO::color_cyan() { Print("\033[36m"); }
void IO::color_white() { Print("\033[37m"); }
