#include "train.h"

static void send_command(int data, int address)
{
   uart_putc(MARKLIN, (uint8_t)data);
   uart_putc(MARKLIN, (uint8_t)address);
   clock_delay(30);
}

void turn_on_headlight(int train_num)
{
   int data = 16;
   send_command(data, train_num);
}

void accelerate_train(int train_num, int speed, bool headlightOn)
{
   int data = speed + (headlightOn ? 16 : 0);
   send_command(data, train_num);
}

void reverse_train(int train_num)
{
   send_command(15, train_num);
}

void stop_train(int train_num)
{
   send_command(0, train_num);
}
