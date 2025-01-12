#include "switch.h"
#include <stdint.h>

#define STRAIGHT 0x21
#define CURVED 0x22
#define OFF 0x20

Switch switches[18] = {0}; // Zero-initialize the entire array

static void send_command(int data, int address)
{
   uart_putc(MARKLIN, (uint8_t)data);
   uart_putc(MARKLIN, (uint8_t)address);
   clock_delay(200);
}

void switch_straight(int switch_number)
{
   send_command(STRAIGHT, switch_number);
   send_command(OFF, switch_number);
}

void switch_branch(int switch_number)
{
   send_command(CURVED, switch_number);
   send_command(OFF, switch_number);
}

void set_all_switches(bool isStraight)
{
   for (int i = 0; i < 18; ++i)
   {
      switches[i].address = i + 1;
      switches[i].straight = isStraight;
      if (isStraight)
      {
         switch_straight(i + 1);
      }
      else
      {
         switch_branch(i + 1);
      }
   }
}