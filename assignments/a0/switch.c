#include "switch.h"
#include "track_node.h"
#include <stdint.h>

#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define OFF 0x20

static bool UPDATE_DISPLAY = true;

Switch switches[22] = {0}; // Zero-initialize the entire array
static const int MIDDLE_SWITCH_ADDRS[4] = {0x9A, 0x9B, 0x9C, 0x99};

static int find_index(int addr)
{
   int index = -1;
   if (addr < 19)
   {
      return addr - 1;
   }

   for (int i = 18; i < 22; ++i)
   {
      if (switches[i].address == addr)
      {
         index = i;
         break;
      }
   }
   return index;
}

static void add_color()
{
   uart_puts(CONSOLE, "\033[35m"); // set colour to cyan
}

static void send_command(int data, int address)
{
   uart_putc(MARKLIN, (uint8_t)data);
   uart_putc(MARKLIN, (uint8_t)address);
   clock_delay(200);
}

void switch_straight(int switch_number)
{
   int idx = find_index(switch_number);
   if (idx != -1)
   {
      switches[idx].straight = STRAIGHT;
      send_command(STRAIGHT_CMD, switch_number);
      send_command(OFF, switch_number);
      UPDATE_DISPLAY = true;
   }
}

void switch_branch(int switch_number)
{
   int idx = find_index(switch_number);
   if (idx != -1)
   {
      switches[idx].straight = CURVED;
      send_command(CURVED_CMD, switch_number);
      send_command(OFF, switch_number);
      UPDATE_DISPLAY = true;
   }
}

static void set_middle_switches()
{
   // add 18
   for (int i = 0; i < 4; ++i)
   {
      int idx = i + 18;
      int addr = MIDDLE_SWITCH_ADDRS[i];

      switches[idx].address = addr;
      switches[idx].straight = (i % 2) ? STRAIGHT : CURVED;
      if (i % 2)
      {
         switch_straight(addr);
      }
      else
      {
         switch_branch(addr);
      }
   }
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
   // initialize middle ones
   set_middle_switches();
}

void init_switch_display(int LOCATION)
{
   reset_formatting(CONSOLE);
   move_cursor(CONSOLE, LOCATION, 1);
   add_color();
   uart_printf(CONSOLE, "--------------------------\r\n");
   uart_printf(CONSOLE, "|   Table of Switches:   |\r\n");
   uart_printf(CONSOLE, "--------------------------\r\n");
   uart_printf(CONSOLE, "|  Switch  |   Status    |\r\n");
   uart_printf(CONSOLE, "--------------------------\r\n");
}

// pass switchlocation + 5
void switch_display(int LOCATION)
{
   if (!UPDATE_DISPLAY)
      return;
   reset_formatting(CONSOLE);
   move_cursor(CONSOLE, LOCATION, 1);
   add_color();
   for (int i = 0; i < 22; ++i)
   {
      clear_to_end_line(CONSOLE);
      uart_printf(CONSOLE, "|     %03d    |     %c      |\r\n", switches[i].address, (switches[i].straight == STRAIGHT) ? 'S' : 'C');
   }
   uart_printf(CONSOLE, "--------------------------\r\n");
   UPDATE_DISPLAY = false;
}

bool is_valid_switch(int switch_number)
{
   return (find_index(switch_number) < 0) ? false : true;
}

// Map track node switch numbers to actual Marklin addresses
int switch_num_to_address(int track_node_num) {
   // Handle special middle switches (Track B specific)
   return track_node_num;
}

void set_switches(const SwitchSetting* switches_set, int num_switches) {
   for(int i = 0; i < num_switches; i++) {
       int address = switch_num_to_address(switches_set[i].switch_num);
       
       if(!is_valid_switch(address)) {
           //uart_printf(CONSOLE, "Warning: Invalid switch number %d (mapped to 0x%x)\r\n",
           //           switches_set[i].switch_num, address);
           continue;
       }

       //uart_printf(CONSOLE, "Setting switch %d (addr 0x%x) to %s\r\n",
       //           switches_set[i].switch_num, address,
       //           switches_set[i].dir == SWITCH_STRAIGHT ? "straight" : "curved");

       if(switches_set[i].dir == SWITCH_STRAIGHT) {
           switch_straight(address);
       } else {
           switch_branch(address);
       }
       
       // Add verification delay
       clock_delay(200);
   }
   // UPDATE_DISPLAY = true;
}