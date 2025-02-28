#include "../include/ui.h"
#include "../include/usertask.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"
#include "../include/io.h"
namespace UI_NS
{

    UI::UI()
    {
        // int marklin_tid = WHOIS("MarklinIOServer");
        // MARKLIN_IO_SERVER::Getc(, MARKLIN);
        // io.reset_formatting();
        // io.clear_screen();
        IO_NS::Print(COLOR_GREEN MOVE_CURSOR "WELCOME TO THE TRAIN CONTROLLER\r\n", 0, 0);
        clock.Display();

        // Switches
        Switch_NS::InitDisplay();

        // Sensors
        // init_sensor_display(SENSOR_LOCATION);

        // CommandPrompt (HANDLED BY OWN TASK)

        // uart_puts(CONSOLE, "Press 'q' to reboot\r\n");
    }
    void UI::Update()
    {
        clock.Update();
        clock.Display();

        Switch_NS::Display();
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
