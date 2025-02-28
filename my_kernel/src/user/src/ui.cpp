#include "../include/ui.h"
#include "../include/usertask.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"
#include "../include/io.h"
#include "../../kern/syscall.h"
namespace UI_NS
{
    IdleTask::IdleTask()
    {
        int idlePercent = 0;
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "IDLE: %d%%", IDLE_LOCATION, 0, idlePercent);
    }
    IdleTask::~IdleTask()
    {
    }
    void IdleTask::Display()
    {
        uint32_t idlePercent = GETIDLEPERCENT();
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "%d%%", IDLE_LOCATION, 7, idlePercent);
    }

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
        idleTask.Display();

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
