#include <cstring>
#include "rpi.h"

typedef void (*funcvoid0_t)();

extern char __bss_start, __bss_end;								// defined in linker script
extern funcvoid0_t __init_array_start, __init_array_end; // defined in linker script

// void* __dso_handle = 0;

extern "C" void setup_mmu();

extern "C" int kmain()
{
#if defined(MMU)
	setup_mmu();
#endif

	// clear out BSS segment
	memset(&__bss_start, 0, &__bss_end - &__bss_start);

	// call global constructors
	for (funcvoid0_t *ctr = &__init_array_start; ctr < &__init_array_end; ctr += 1)
		(*ctr)();

	// set up GPIO pins for both console and marklin uarts
	gpio_init();
	// not strictly necessary, since console is configured during boot
	uart_config_and_enable(CONSOLE);
	// welcome message
	uart_puts(CONSOLE, "\r\nHello world, this is version: " __DATE__ " / " __TIME__ "\r\n\r\nPress 'q' to reboot\r\n");

	unsigned int counter = 1;
	for (;;)
	{
		uart_printf(CONSOLE, "PI[%u]> ", counter++);
		for (;;)
		{
			char c = uart_getc(CONSOLE);
			uart_putc(CONSOLE, c);
			if (c == '\r')
			{
				uart_putc(CONSOLE, '\n');
				break;
			}
			else if (c == 'q' || c == 'Q')
			{
				uart_puts(CONSOLE, "\r\n");
				return 0;
			}
		}
	}
}

#if !defined(MMU)
#include <cstddef>

// define our own memset to avoid SIMD instructions emitted from the compiler
extern "C" void *memset(void *s, int c, size_t n)
{
	for (char *it = (char *)s; n > 0; --n)
		*it++ = c;
	return s;
}

// define our own memcpy to avoid SIMD instructions emitted from the compiler
extern "C" void *memcpy(void *dest, const void *src, size_t n)
{
	char *sit = (char *)src;
	char *cdest = (char *)dest;
	for (size_t i = 0; i < n; ++i)
		*cdest++ = *sit++;
	return dest;
}
#endif
