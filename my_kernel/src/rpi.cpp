#include "rpi.h"
#include "util.h"
#include <stdarg.h>
#include <stdint.h>

static char *const MMIO_BASE = (char *)0xFE000000;

/*********** GPIO CONFIGURATION ********************************/
// Offsets can be found at page 67: https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf
static char *const GPIO_BASE = (char *)(MMIO_BASE + 0x200000);
static const uint32_t GPFSEL_OFFSETS[6] = {0x00, 0x04, 0x08, 0x0c, 0x10, 0x14}; // GPIO Function Select Register
static const uint32_t GPIO_PUP_PDN_CNTRL_OFFSETS[4] = {0xe4, 0xe8, 0xec, 0xf0}; // GPIO Pull-up/down Register

#define GPFSEL_REG(reg) (*(uint32_t *)(GPIO_BASE + GPFSEL_OFFSETS[reg]))
#define GPIO_PUP_PDN_CNTRL_REG(reg) (*(uint32_t *)(GPIO_BASE + GPIO_PUP_PDN_CNTRL_OFFSETS[reg]))

// function control settings for GPIO pins (page 68)
static const uint32_t GPIO_INPUT = 0x00;
static const uint32_t GPIO_OUTPUT = 0x01;
static const uint32_t GPIO_ALTFN0 = 0x04;
static const uint32_t GPIO_ALTFN1 = 0x05;
static const uint32_t GPIO_ALTFN2 = 0x06;
static const uint32_t GPIO_ALTFN3 = 0x07;
static const uint32_t GPIO_ALTFN4 = 0x03;
static const uint32_t GPIO_ALTFN5 = 0x02;

// pup/pdn resistor settings for GPIO pins
static const uint32_t GPIO_NONE = 0x00;
static const uint32_t GPIO_PUP = 0x01;
static const uint32_t GPIO_PDP = 0x02;

static void setup_gpio(uint32_t pin, uint32_t setting, uint32_t resistor)
{
    uint32_t reg = pin / 10;
    uint32_t shift = (pin % 10) * 3;
    uint32_t status = GPFSEL_REG(reg); // read status
    status &= ~(7u << shift);          // clear bits
    status |= (setting << shift);      // set bits
    GPFSEL_REG(reg) = status;

    reg = pin / 16;
    shift = (pin % 16) * 2;
    status = GPIO_PUP_PDN_CNTRL_REG(reg); // read status
    status &= ~(3u << shift);             // clear bits
    status |= (resistor << shift);        // set bits
    GPIO_PUP_PDN_CNTRL_REG(reg) = status; // write back
}

// GPIO initialization, to be called before UART functions.
// For UART3 (line 2 on the RPi hat), we need to configure the GPIO to route
// the uart control and data signals to the GPIO pins 4-7 expected by the hat.
// GPIO pins 14 & 15 already configured by boot loader, but redo for clarity.
void gpio_init()
{
    setup_gpio(4, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(5, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(6, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(7, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(14, GPIO_ALTFN0, GPIO_NONE);
    setup_gpio(15, GPIO_ALTFN0, GPIO_NONE);
}

/*********** UART CONTROL ************************ ************/

static char *const UART0_BASE = (char *)(MMIO_BASE + 0x201000);
static char *const UART3_BASE = (char *)(MMIO_BASE + 0x201600);

static char *const line_uarts[] = {NULL, UART0_BASE, UART3_BASE};
#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))

// UART register offsets
static const uint32_t UART_DR = 0x00;   // data register
static const uint32_t UART_RSR = 0x04;  // receive status register
static const uint32_t UART_FR = 0x18;   // flag register
static const uint32_t UART_IBRD = 0x24; // integer baud rate divisor
static const uint32_t UART_FBRD = 0x28; // fractional baud rate divisor
static const uint32_t UART_LCRH = 0x2c; // line control register
static const uint32_t UART_CR = 0x30;   // control register

// masks for specific fields in the UART registers

// flag register masks
// 0x10 -> 0001 0000 -> set bit 4
static const uint32_t UART_FR_RXFE = 0x10; // receive FIFO empty
static const uint32_t UART_FR_TXFF = 0x20; // transmit FIFO full
static const uint32_t UART_FR_RXFF = 0x40; // receive FIFO full
static const uint32_t UART_FR_TXFE = 0x80; // transmit FIFO empty

// control register masks
static const uint32_t UART_CR_UARTEN = 0x01;  // UART enable
static const uint32_t UART_CR_LBE = 0x80;     // loopback enable
static const uint32_t UART_CR_TXE = 0x100;    // transmit enable
static const uint32_t UART_CR_RXE = 0x200;    // receive enable
static const uint32_t UART_CR_RTS = 0x800;    // request to send
static const uint32_t UART_CR_RTSEN = 0x4000; // RTS enable
static const uint32_t UART_CR_CTSEN = 0x8000; // CTS hardware flow control enable

// line control register masks
static const uint32_t UART_LCRH_PEN = 0x2;        // parity enable
static const uint32_t UART_LCRH_EPS = 0x4;        // even parity select
static const uint32_t UART_LCRH_STP2 = 0x8;       // two stop bits select
static const uint32_t UART_LCRH_FEN = 0x10;       // enable FIFOs
static const uint32_t UART_LCRH_WLEN_LOW = 0x20;  // word length low bit
static const uint32_t UART_LCRH_WLEN_HIGH = 0x40; // word length high bit

// Configure the line properties (e.g, parity, baud rate) of a UART and ensure that it is enabled
void uart_config_and_enable(size_t line)
{
    uint32_t baud_ival, baud_fval;
    uint32_t stop2;

    switch (line)
    {
    // setting baudrate to approx. 115246.09844 (best we can do); 1 stop bit
    case CONSOLE:
        baud_ival = 26;
        baud_fval = 2;
        stop2 = 0;
        break;
    // setting baudrate to 2400; 2 stop bits
    case MARKLIN:
        baud_ival = 1250;
        baud_fval = 0;
        stop2 = UART_LCRH_STP2;
        break;
    default:
        return;
    }

    // line control registers should not be changed while the UART is enabled, so disable it
    uint32_t cr_state = UART_REG(line, UART_CR);
    UART_REG(line, UART_CR) = cr_state & ~UART_CR_UARTEN; // ~UART_CR_UARTEN -> 0xFFFE

    // set the baud rate
    UART_REG(line, UART_IBRD) = baud_ival;
    UART_REG(line, UART_FBRD) = baud_fval;

    // set the line control registers: 8 bit, no parity, 1 or 2 stop bits, FIFOs enabled
    UART_REG(line, UART_LCRH) = UART_LCRH_WLEN_HIGH | UART_LCRH_WLEN_LOW | UART_LCRH_FEN | stop2;

    // re-enable the UART; enable both transmit and receive regardless of previous state
    UART_REG(line, UART_CR) = cr_state | UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
}

// non-blocking version to check if buffer is filled with data
int uart_getc_non_blocking(size_t line)
{
    // Check the Receiver FIFO Empty (RXFE) bit in the UART Flag Register
    if (UART_REG(line, UART_FR) & UART_FR_RXFE)
    {
        return NO_CHAR;
    }
    unsigned char ch = UART_REG(line, UART_DR);
    return ch;
}

unsigned char uart_getc(size_t line)
{
    unsigned char ch;
    /* wait for data if necessary */
    while (UART_REG(line, UART_FR) & UART_FR_RXFE)
    {
        // wait for data
        // uart_printf(CONSOLE, "SPINNING DETECTED\r\n");
    }
    ch = UART_REG(line, UART_DR);
    return ch;
}

// declare in header file
bool uart_transmit_c(size_t line)
{
    // // check if transmit FIFO is full
    // if (UART_REG(line, UART_FR) & UART_FR_TXFF)
    // {
    //     return false;
    // }
    // return true;
}

// should only call when there is data to read (after awaiting event)
unsigned char uart_receive_c(size_t line)
{
    unsigned char ch;
    // check
    ch = UART_REG(line, UART_DR);
    // obtain status UARTRSR
    auto status = UART_REG(line, UART_RSR); // cannot be read until data is read from the DR
    if (status & 0xF != 0)
    {
        UART_REG(line, UART_RSR) = 1; // clear the error
    }
    return ch;
}

void uart_putc(size_t line, char c)
{
    // make sure there is room to write more data
    while (UART_REG(line, UART_FR) & UART_FR_TXFF)
    {
        // uart_printf(CONSOLE, "SPINNING DETECTED\r\n");
    }
    UART_REG(line, UART_DR) = c;
}

void uart_putl(size_t line, const char *buf, size_t blen)
{
    for (size_t i = 0; i < blen; i++)
    {
        uart_putc(line, *(buf + i));
    }
}

void uart_puts(size_t line, const char *buf)
{
    while (*buf)
    {
        uart_putc(line, *buf);
        buf++;
    }
}

// Helper function to handle padding
static void pad_and_print(size_t line, const char *str, int width)
{
    int len = 0;
    const char *temp = str;

    // Calculate string length
    while (*temp++)
        len++;

    // Add padding spaces if necessary
    while (len < width)
    {
        uart_putc(line, ' ');
        width--;
    }

    // Print the actual string
    uart_puts(line, str);
}

extern "C" void uart_printf(size_t line, const char *fmt, ...)
{
    va_list va;
    char ch, buf[12];
    int width = 0; // Width for padding

    va_start(va, fmt);
    while ((ch = *(fmt++)))
    {
        if (ch != '%')
        {
            uart_putc(line, ch); // Print regular characters
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
                pad_and_print(line, buf, width);
                break;

            case 'd': // Signed integer
                i2a(va_arg(va, int), buf);
                pad_and_print(line, buf, width);
                break;

            case 'x': // Hexadecimal
                ui2a(va_arg(va, unsigned int), 16, buf);
                pad_and_print(line, buf, width);
                break;

            case 's': // String
            {
                char *str = va_arg(va, char *);
                pad_and_print(line, str, width);
                break;
            }

            case 'c':                       // Character
                ch = (char)va_arg(va, int); // Characters are promoted to int in varargs
                if (width > 1)
                {
                    for (int i = 1; i < width; ++i) // Add padding spaces
                        uart_putc(line, ' ');
                }
                uart_putc(line, ch); // Print the character
                break;

            case '%': // Literal '%'
                uart_putc(line, '%');
                break;

            case '\0': // End of format string
                return;

            default:
                // Handle unknown specifier by printing it
                uart_putc(line, '%');
                uart_putc(line, ch);
                break;
            }
        }
    }
    va_end(va);
}
