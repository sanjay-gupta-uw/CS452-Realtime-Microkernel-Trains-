#include "../include/io.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include <stdarg.h>
#include "../../util.h"
// #include "../../rpi.h"
#include "../include/uassert.h"
namespace IO_NS
{
#define CONSOLE 1
#define MARKLIN 2
#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif

    static int IO_SERVER_TID = -1;

    IO::IO()
    {
        IO_SERVER_TID = WHOIS("IOServer");
        uassert(IO_SERVER_TID > 0 && "IO_NS::Error finding IO Server");
    }

    IO::~IO()
    {
    }

    static void fill_buffer_char(char *buffer, char ch, int *len)
    {
        buffer[(*len)++] = ch;
        buffer[*len] = '\0';
    }

    static void fill_buffer_wrapper(char *buffer, const char *str, int *len)
    {
        while (*str)
        {
            fill_buffer_char(buffer, *str, len);
            str++;
        }
    }

    // DISABLED PRINT FOR TESTING
    extern "C" void Print(const char *fmt, ...)
    {
        if (IO_SERVER_TID < 0)
        {
            return;
        }
        char ret_buffer[RET_BUF_SIZE];
        va_list va;
        char ch, buf[12];
        int len = 0; // Width for padding

        // PREPEND WITH COLOR_WHITE
        fill_buffer_wrapper(ret_buffer, COLOR_WHITE, &len);

        va_start(va, fmt);
        while ((ch = *(fmt++)))
        {
            if (ch != '%')
            {
                fill_buffer_char(ret_buffer, ch, &len);
            }
            else
            {
                // Parse padding width if present
                ch = *(fmt++);
                switch (ch)
                {
                case 'u': // Unsigned integer
                    ui2a(va_arg(va, unsigned int), 10, buf);
                    fill_buffer_wrapper(ret_buffer, buf, &len);
                    break;

                case 'd': // Signed integer
                    i2a(va_arg(va, int), buf);
                    fill_buffer_wrapper(ret_buffer, buf, &len);
                    break;

                case 'x': // Hexadecimal
                    ui2a(va_arg(va, unsigned int), 16, buf);
                    fill_buffer_wrapper(ret_buffer, buf, &len);
                    break;

                case 's': // String
                {
                    char *str = va_arg(va, char *);
                    fill_buffer_wrapper(ret_buffer, str, &len);
                    break;
                }

                case 'c':                       // Character
                    ch = (char)va_arg(va, int); // Characters are promoted to int in varargs
                    fill_buffer_char(ret_buffer, ch, &len);
                    break;

                case '%':
                case '\0': // End of format string
                    fill_buffer_char(ret_buffer, ch, &len);
                    break;

                default:
                    // Handle unknown specifier by printing it
                    break;
                }
            }
        }
        va_end(va);
        if (len > 0)
        {
            // check debug mode
#if IRQ_ENABLED == 1
            IO_SERVER::Puts(IO_SERVER_TID, (unsigned char *)ret_buffer);
#else
            uart_puts(CONSOLE, ret_buffer);
#endif
        }
    }

    extern "C" void PrintTerminal(const char *fmt, ...)
    {
        if (IO_SERVER_TID < 0)
        {
            return;
        }

        char ret_buffer[RET_BUF_SIZE];
        va_list va;
        char ch, buf[12];
        int len = 0;

        // Save cursor position before printing
        fill_buffer_wrapper(ret_buffer, RESTORE_CURSOR, &len);
        fill_buffer_wrapper(ret_buffer, COLOR_WHITE, &len);

        va_start(va, fmt);
        while ((ch = *(fmt++)))
        {
            if (ch != '%')
            {
                fill_buffer_char(ret_buffer, ch, &len);
            }
            else
            {
                ch = *(fmt++);
                switch (ch)
                {
                case 'u': // Unsigned integer
                    ui2a(va_arg(va, unsigned int), 10, buf);
                    fill_buffer_wrapper(ret_buffer, buf, &len);
                    break;

                case 'd': // Signed integer
                    i2a(va_arg(va, int), buf);
                    fill_buffer_wrapper(ret_buffer, buf, &len);
                    break;

                case 'x': // Hexadecimal
                    ui2a(va_arg(va, unsigned int), 16, buf);
                    fill_buffer_wrapper(ret_buffer, buf, &len);
                    break;

                case 's': // String
                {
                    char *str = va_arg(va, char *);
                    fill_buffer_wrapper(ret_buffer, str, &len);
                    break;
                }

                case 'c': // Character
                    ch = (char)va_arg(va, int);
                    fill_buffer_char(ret_buffer, ch, &len);
                    break;

                case '%':
                case '\0': // End of format string
                    fill_buffer_char(ret_buffer, ch, &len);
                    break;

                default:
                    // Unknown specifier
                    break;
                }
            }
        }
        va_end(va);

        // Restore cursor after printing
        fill_buffer_wrapper(ret_buffer, SAVE_CURSOR, &len);

        if (len > 0)
        {
#if IRQ_ENABLED == 1
            IO_SERVER::Puts(IO_SERVER_TID, (unsigned char *)ret_buffer);
#else
            uart_puts(CONSOLE, ret_buffer);
#endif
        }
    }

}
