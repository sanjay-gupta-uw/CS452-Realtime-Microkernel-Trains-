#include "train.h"

TrainComponent trains[5] = {
    {1, 0, false},
    {54, 0, false},
    {55, 0, false},
    {58, 0, false},
    {77, 0, false},
};

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

void accelerate_train(int train_num, int speed, bool headlightOn, int idx)
{
   if (speed == -1)
   {
      speed = trains[idx].train_speed;
   }
   int data = speed + (headlightOn ? 16 : 0);
   trains[idx].train_speed = speed;
   send_command(data, train_num);
}

void reverse_train(int train_num)
{
   int data = 15;
   send_command(data, train_num);
}

void stop_train(int train_num)
{
   send_command(0, train_num);
}

int is_valid_train(int train_num)
{
   for (int i = 0; i < 5; ++i)
   {
      if (trains[i].train_num == train_num)
         return i;
   }
   return -1;
}

bool is_moving(int idx)
{
   return (trains[idx].train_speed > 0);
}