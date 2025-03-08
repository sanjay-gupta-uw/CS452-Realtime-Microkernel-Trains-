#include "../../clock.h"
#include "../../include/syscall.h"
#include "../marklin/switch.h"
#include "../marklin/sensor.h"
#include "../include/ui.h"
#include "../include/io.h"

namespace UI_NS
{

    static void init_switch_display()
    {
        const int SWITCH_ADDR[SWITCH_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0x9A, 0x9B, 0x9C, 0x99};
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-------------------------\r\n", SWITCH_LOCATION + 0, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|   Table of Switches:  |\r\n", SWITCH_LOCATION + 1, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-------------------------\r\n", SWITCH_LOCATION + 2, 1);
        // IO_NS::Print(COLOR_WHITE MOVE_CURSOR  "|  Switch  |   Status    |\r\n", SWITCH_LOCATION + 3, 1);
        // IO_NS::Print(COLOR_WHITE MOVE_CURSOR  "--------------------------\r\n", SWITCH_LOCATION + 4, 1);
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            // print borders and switch address so display only needs to update the status
            int location_y = SWITCH_LOCATION + 3 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|" MOVE_CURSOR "%d" MOVE_CURSOR "|" MOVE_CURSOR "|",
                         location_y, 1, location_y, 6, SWITCH_ADDR[i], location_y, 12, location_y, 25);
        }
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-------------------------\r\n", SWITCH_LOCATION + 3 + SWITCH_COUNT, 1);
    }

    static void init_sensor_display()
    {
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "------------------------\r\n", SENSOR_LOCATION + 0, 1 + BOX_WIDTH);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "   Table of Sensors:   |\r\n", SENSOR_LOCATION + 1, 1 + BOX_WIDTH);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "------------------------\r\n", SENSOR_LOCATION + 2, 1 + BOX_WIDTH);
        for (int i = 0; i < MAX_RECENT_SENSORS; ++i)
        {
            // print borders and switch address so display only needs to update the status
            int location_y = SENSOR_LOCATION + 3 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|", location_y, 2 * BOX_WIDTH - 1);
        }
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "------------------------\r\n", SENSOR_LOCATION + 3 + MAX_RECENT_SENSORS, 1 + BOX_WIDTH);
    }

    void start_ui()
    {
        REGISTERAS("UI_TASK");
        Clock clock;

        // sensors.setMarklinIOtid(MARKLIN_IO_SERVER_TID);

        for (int i = 1; i <= NUM_COLS; i++)
        {
            IO_NS::Print(MOVE_CURSOR "-", SCROLL_ROW_END, i);
        }

        init_switch_display();
        IO_NS::Print(MOVE_CURSOR "IDLE: %d%%", IDLE_LOCATION, 1, 1);
        clock.Display();
        init_sensor_display();

        EXIT();
    }

}