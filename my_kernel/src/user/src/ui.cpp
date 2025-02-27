#include "../include/ui.h"
#include "../include/usertask.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"
namespace UI_NS
{

    UI::UI()
    {
        // int marklin_tid = WHOIS("MarklinIOServer");
        // MARKLIN_IO_SERVER::Getc(, MARKLIN);
        // io.reset_formatting();
        // io.clear_screen();
        io.Print(COLOR_GREEN MOVE_CURSOR "WELCOME TO THE TRAIN CONTROLLER\r\n", 0, 0);

        clock.Display(&io, CLOCK_LOCATION_X, CLOCK_LOCATION_Y);

        // Switches
        switches.InitDisplay(&io, SWITCH_LOCATION);

        // Sensors
        // init_sensor_display(SENSOR_LOCATION);

        // CommandPrompt (HANDLED BY OWN TASK)

        // uart_puts(CONSOLE, "Press 'q' to reboot\r\n");
    }
    void UI::Update()
    {
        clock.Update();
        clock.Display(&io, CLOCK_LOCATION_X, CLOCK_LOCATION_Y);

        switches.Display(&io, SWITCH_LOCATION + 5);
    }

    void start_ui()
    {
        UI ui;

        while (true)
        {
            ui.Update();
        }
    }
}
