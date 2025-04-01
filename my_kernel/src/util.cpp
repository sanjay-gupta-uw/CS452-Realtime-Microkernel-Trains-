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
uint32_t a2ui(char **src, unsigned int base)
{
    unsigned int num;
    int digit;
    char *p;

    p = *src; //
    num = 0;

    char ch = *p;
    while ((digit = a2d(ch)) >= 0)
    {
        if ((unsigned int)digit > base)
            break;
        num = num * base + digit;
        ch = *++p;
    }
    // *src = p;
    return num;
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

int get_index_from_name(const char *name)
{
    int index = 0;
    char *ptr = (const_cast<char *>(name));
    char bank = *ptr++;
    int id = a2ui(&ptr, 10);
    return ((bank - 'A') * 16) + (id - 1);
}