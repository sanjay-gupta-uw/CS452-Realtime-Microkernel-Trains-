#include "../include/ui.h"
#include "../include/usertask.h"
namespace UI_NS
{

    UI::UI()
    {
        io.clear_screen();
        io.color_green();
        io.Print("WELCOME TO THE TRAIN CONTROLLER\r\n");
        // clock.Display((NUM_COLS - CLOCK_LENGTH) / 2);

        // clock.Display((NUM_COLS - CLOCK_LENGTH) / 2);

        // Switches
        switches.InitDisplay(&io, SWITCH_LOCATION);

        // Sensors
        // init_sensor_display(SENSOR_LOCATION);

        // uart_puts(CONSOLE, "Press 'q' to reboot\r\n");
    }
    void UI::Update()
    {
        switches.Display(&io, SWITCH_LOCATION + 5);
    }

    // void CommandPrompt::Display()
    // {
    //     clear_screen();
    //     int LOCATION_Y = 1;
    //     // DISPLAY CLOCK

    //     move_cursor(LOCATION_Y, 1);
    //     // io.Print("WELCOME TO THE COMMAND PROMPT\r\n");
    //     // move_cursor(1, 1);
    //     io.Print("IDLE TIME:\r\n");

    //     move_cursor(LOCATION_Y + 2, 1);
    //     //

    //     color_green();
    //     io.Print("$: \r\n");
    //     UPDATE_DISPLAY = false;
    // }
    // ********** CommandPrompt Class End **********

    void start_ui()
    {
        // IO io;
        // io.clear_screen();
        // io.Print("Starting UI\r\n");
        UI ui;

        while (true)
        {
            ui.Update();
            // update();
        }
    }
}
