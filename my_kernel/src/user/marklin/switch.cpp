#include "switch.h"
// #include "../clock.h"
#include "../include/io.h"
// #include "../include/marklin_io.h"

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

    // MARKLIN_IO_SERVER::Putc(MARKLIN, (uint8_t)data);
    // MARKLIN_IO_SERVER::Putc(MARKLIN, (uint8_t)address);
    // uart_putc(MARKLIN, (uint8_t)data);
    // uart_putc(MARKLIN, (uint8_t)address);
    // clock.Delay(200);
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

void Switch::Print(IO *io) const
{
    // // uart_printf(CONSOLE, "|     %03d    |     %c      |\r\n", address, (ALIGNMENT == STRAIGHT) ? 'S' : 'C');
    io->Print("|     %03d    |", address);

    if (ALIGNMENT == STRAIGHT)
    {
        io->color_green();
        io->Print("    %c      |\r\n", 'S');
        // uart_printf(CONSOLE, "     %c      |\r\n", address, 'S');
    }
    else
    {
        io->color_red();
        uart_printf(CONSOLE, "     %c      |\r\n", 'C');
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

void Switches::InitDisplay(IO *io, int LOCATION)
{
    io->move_cursor(LOCATION, 1);
    io->Print("--------------------------\r\n");
    io->Print("|   Table of Switches:   |\r\n");
    io->Print("--------------------------\r\n");
    io->Print("|  Switch  |   Status    |\r\n");
    io->Print("--------------------------\r\n");
}

void Switches::Display(IO *io, int LOCATION)
{
    // if (!UPDATE_DISPLAY)
    //     return;

    io->move_cursor(LOCATION, 1);
    for (int i = 0; i < 22; ++i) // change to update specfic switch
    {
        io->clear_to_end_line();
        switches[i].Print(io);
    }
    // uart_printf(CONSOLE, "--------------------------\r\n");
    UPDATE_DISPLAY = false;
}

// ********* End Switches Class *********