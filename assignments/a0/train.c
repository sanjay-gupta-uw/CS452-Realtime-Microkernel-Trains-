#include "train.h"
#include "rpi.h"

static void send_command(uint8_t data, uint8_t address)
{
   uint8_t cmd[2];
   cmd[0] = data;
   cmd[1] = address;

   uart_putl(MARKLIN, (char *)cmd, 2);
   uart_putc(MARKLIN, '\r');
}

void turn_on_headlight(uint8_t train_num)
{
   uint8_t data = 16;
   send_command(data, train_num);
}