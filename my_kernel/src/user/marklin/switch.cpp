#include "switch.h"
// #include "../clock.h"
// #include "../include/io.h"
// #include "../include/name_server.h"
// #include "../include/marklin_io.h"
#include "../include/clock_server.h"

#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define OFF 0x20

static bool UPDATE_DISPLAY = true;
static int CLOCK_SERVER_TID = -1;
static int MARKLIN_IO_SERVER_TID = -1;

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
    // MARKLIN_IO_SERVER::Putc(MARKLIN_IO_SERVER_TID, MARKLIN, (unsigned char)data);
    // MARKLIN_IO_SERVER::Putc(MARKLIN_IO_SERVER_TID, MARKLIN, (unsigned char)address);
    // DELAY(CLOCK_SERVER_TID, 15); // use clock server

    // MARKLIN_IO_SERVER::MarklinRequest request{MARKLIN_IO_SERVER::MARKLIN_REQUEST_TYPE::SET_SWITCH, address, data};
}

void Switch::SetSwitch(SWITCH_STATE ALIGNMENT)
{

    // SendCommand(ALIGNMENT == STRAIGHT ? STRAIGHT_CMD : CURVED_CMD);
    // SendCommand(OFF);
    if (this->ALIGNMENT != ALIGNMENT)
    {
        this->ALIGNMENT = ALIGNMENT;
        UPDATE_DISPLAY = true;
    }
}

// void Switch::Print(IO *io) const
// {
//     // io->Print(COLOR_WHITE "|     %03d    |" COLOR_RED "    %c      " COLOR_WHITE "|\r\n", address, ALIGNMENT == STRAIGHT ? 'S' : 'C');
// }

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

// ensures valid address passed in
bool Switches::SetSwitch(MarklinRequest *request)
{
    // int addr = request->id;
    // char state = (char)request->data;
    // SWITCH_STATE ALIGNMENT = (state == 'S' || state == 's') ? STRAIGHT : CURVED;
    // for (int i = 0; i < 22; ++i)
    // {
    //     if (switches[i].address == addr)
    //     {
    //         // switches[i].SetSwitch(ALIGNMENT);
    //         // request->id = addr;
    //         *(request->data) = ALIGNMENT;

    //         return true;
    //     }
    // }
    // return false;
}

// DEPRECATED
void Switches::SetAll(SWITCH_STATE ALIGNMENT)
{
    // for (int i = 0; i < 22; ++i)
    // {
    //     switches[i].SetSwitch(ALIGNMENT);
    // }
}

void Switches::InitDisplay(int LOCATION)
{
    IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", LOCATION + 0, 1);
    IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|   Table of Switches:   |\r\n", LOCATION + 1, 1);
    IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", LOCATION + 2, 1);
    IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|  Switch  |   Status    |\r\n", LOCATION + 3, 1);
    IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", LOCATION + 4, 1);
}

void Switches::Display(int LOCATION)
{
    // if (!UPDATE_DISPLAY)
    //     return;
    for (int i = 0; i < 22; ++i) // change to update specfic switch
    {
        Switch *s = &switches[i];
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE COLOR_WHITE "|     %d    |" COLOR_RED "    %c      " COLOR_WHITE "|\r\n", LOCATION + i, 1, s->address, s->ALIGNMENT == STRAIGHT ? 'S' : 'C');
    }
    // uart_printf(CONSOLE, "--------------------------\r\n");
    UPDATE_DISPLAY = false;
}

void Switches::setClockServerTid(int tid)
{
    CLOCK_SERVER_TID = tid;
}

void Switches::setMarklinIoServerTid(int tid)
{
    MARKLIN_IO_SERVER_TID = tid;
}
// ********* End Switches Class *********