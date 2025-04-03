// #include "../../clock.h"
#include "../../include/syscall.h"
#include "../marklin/switch.h"
#include "../marklin/sensor.h"
#include "../include/ui.h"
#include "../include/io.h"
#include "../include/uassert.h"
namespace UI_NS
{

    static void init_switch_display()
    {
        const int SWITCH_ADDR[NUM_SWITCHES] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               11, 12, 13, 14, 15, 16, 17, 18,
                                               0x99, 0x9A, 0x9B, 0x9C};

        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+------------------------\r\n", SWITCH_LOCATION + 0, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|   Table of Switches:  |\r\n", SWITCH_LOCATION + 1, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+------------------------\r\n", SWITCH_LOCATION + 2, 1);
        // IO_NS::Print(COLOR_WHITE MOVE_CURSOR  "|  Switch  |   Status    |\r\n", SWITCH_LOCATION + 3, 1);
        // IO_NS::Print(COLOR_WHITE MOVE_CURSOR  "--------------------------\r\n", SWITCH_LOCATION + 4, 1);
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            // inal("Switch %d: %d\r\n", i, SWITCH_ADDR[i]);
            // print borders and switch address so display only needs to update the status
            int location_y = SWITCH_LOCATION + 3 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|", location_y, 1);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "%d", location_y, 6, SWITCH_ADDR[i]);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|", location_y, 12);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|", location_y, 25);
        }
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+------------------------\r\n", SWITCH_LOCATION + 3 + SWITCH_COUNT, 1);
    }

    static void init_sensor_display()
    {
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "---------------------------------------------------------------------+\r\n", SENSOR_LOCATION + 0, 1 + BOX_WIDTH);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "   Bank A:   |   Bank B:   |   Bank C:   |   Bank D:   |   Bank E:   |\r\n", SENSOR_LOCATION + 1, 1 + BOX_WIDTH);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "---------------------------------------------------------------------+\r\n", SENSOR_LOCATION + 2, 1 + BOX_WIDTH);
        for (int i = 0; i < 16; ++i)
        {
            // print borders and switch address so display only needs to update the status
            int location_y = SENSOR_LOCATION + 3 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "A%d:" CHANGE_COLUMN "|  B%d:" CHANGE_COLUMN "|  C%d:" CHANGE_COLUMN "|  D%d:" CHANGE_COLUMN "|  E%d:" CHANGE_COLUMN "|", location_y, BOX_WIDTH + 3, i + 1, BOX_WIDTH + 14, i + 1, BOX_WIDTH * 2 + 3, i + 1, BOX_WIDTH * 2 + 17, i + 1, BOX_WIDTH * 3 + 6, i + 1, BOX_WIDTH * 3 + 20);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "%d", SENSOR_START_ROW + i, BANK_A_COL, 0);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "%d", SENSOR_START_ROW + i, BANK_B_COL, 0);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "%d", SENSOR_START_ROW + i, BANK_C_COL, 0);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "%d", SENSOR_START_ROW + i, BANK_D_COL, 0);
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "%d", SENSOR_START_ROW + i, BANK_E_COL, 0);
            // uassert(false && "FORCED PANIC -- UI3 -- REMOVE THIS LINE");
        }

        for (int i = 16; i < SWITCH_COUNT; ++i)
        {
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "             |             |             |             |             |\r\n", SENSOR_LOCATION + 3 + i, 1 + BOX_WIDTH);
        }

        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "---------------------------------------------------------------------+\r\n", SENSOR_LOCATION + 3 + SWITCH_COUNT, 1 + BOX_WIDTH);
    }

    void start_ui()
    {
        // uassert(false && "FORCED PANIC -- UI1 -- REMOVE THIS LINE");
        IO_NS::PrintTerminal("UI TASK STARTED!\r\n");
        // REGISTERAS("UI_TASK");
        // Clock clock;

        // sensors.setMarklinIOtid(MARKLIN_IO_SERVER_TID);
        // uassert(false && "FORCED PANIC -- UI2 -- REMOVE THIS LINE");

        for (int i = 1; i <= NUM_COLS; i++)
        {
            IO_NS::Print(MOVE_CURSOR "-", SCROLL_ROW_END, i);
        }

        init_switch_display();
        IO_NS::Print(MOVE_CURSOR "IDLE: %d%%", IDLE_LOCATION, 1, 0);
        // clock.Display();
        init_sensor_display();
        IO_NS::PrintTerminal("UI TASK INITIALIZED!\r\n");
        EXIT();
    }

}