#include "switch.h"
#include "clock.h"

#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define OFF 0x20

static bool UPDATE_DISPLAY = true;

// Switch switches[22] = {0}; // Zero-initialize the entire array

// ********* Switch Class *********
Switch::Switch()
{
   address = -1;
   ALIGNMENT = CURVED;
}

Switch::~Switch()
{
}

void Switch::SetAddr(int addr)
{
   address = addr;
}

void Switch::SendCommand(int data)
{
   uart_putc(MARKLIN, (uint8_t)data);
   uart_putc(MARKLIN, (uint8_t)address);
   clock.Delay(200);
}

void Switch::SetSwitch(SWITCH_STATE ALIGNMENT)
{
   SendCommand(ALIGNMENT == STRAIGHT ? STRAIGHT_CMD : CURVED_CMD);
   SendCommand(OFF);
   if (this->ALIGNMENT != ALIGNMENT)
   {
      this->ALIGNMENT = ALIGNMENT;
      UPDATE_DISPLAY = true;
   }
}

void Switch::Print() const
{
   // uart_printf(CONSOLE, "|     %03d    |     %c      |\r\n", address, (ALIGNMENT == STRAIGHT) ? 'S' : 'C');
   uart_printf(CONSOLE, "|     %03d    |", address);
   if (ALIGNMENT == STRAIGHT)
   {
      color_green();
      uart_printf(CONSOLE, "     %c      |\r\n", address, 'S');
   }
   else
   {
      color_red();
      uart_printf(CONSOLE, "     %c      |\r\n", address, 'C');
   }
}

// ********* End Switch Class *********

// ********* Switches Class *********
Switches::Switches()
{
   const int MIDDLE_SWITCH_ADDRS[4] = {0x9A, 0x9B, 0x9C, 0x99};
   for (int i = 0; i < 18; ++i)
   {
      switches[i].SetAddr(i + 1);
   }
   for (int i = 18; i < 22; ++i)
   {
      switches[i].SetAddr(MIDDLE_SWITCH_ADDRS[i - 18]);
   }
}

Switches::~Switches()
{
}

bool Switches::SetSwitch(int addr, SWITCH_STATE ALIGNMENT)
{
   for (int i = 0; i < 22; ++i)
   {
      if (switches[i].address == addr)
      {
         switches[i].SetSwitch(ALIGNMENT);
         return true;
      }
   }
   return false;
}

void Switches::SetAll(SWITCH_STATE ALIGNMENT)
{
   for (int i = 0; i < 22; ++i)
   {
      switches[i].SetSwitch(ALIGNMENT);
   }
}

void Switches::InitDisplay(int LOCATION)
{
   move_cursor(CONSOLE, LOCATION, 1);
   uart_printf(CONSOLE, "--------------------------\r\n");
   uart_printf(CONSOLE, "|   Table of Switches:   |\r\n");
   uart_printf(CONSOLE, "--------------------------\r\n");
   uart_printf(CONSOLE, "|  Switch  |   Status    |\r\n");
   uart_printf(CONSOLE, "--------------------------\r\n");
}

void Switches::Display(int LOCATION)
{
   if (!UPDATE_DISPLAY)
      return;

   move_cursor(CONSOLE, LOCATION, 1);
   for (int i = 0; i < 22; ++i)
   {
      clear_to_end_line(CONSOLE);
      switches[i].Print();
   }
   uart_printf(CONSOLE, "--------------------------\r\n");
   UPDATE_DISPLAY = false;
}

// ********* End Switches Class *********