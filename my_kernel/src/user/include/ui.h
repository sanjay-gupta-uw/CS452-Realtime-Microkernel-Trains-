#ifndef _ui_h_
#define _ui_h_

#include "../../clock.h"
#include "../marklin/switch.h"
#include "command.hpp"
// #include "../marklin/train.h"

extern Clock clock;

namespace UI_NS
{

    class IdleTask
    {
        int idlePercent;

    public:
        IdleTask();
        ~IdleTask();
        void Display();
    };

    class UI
    {
        IdleTask idleTask;
        // Switches switches;
        // CommandPrompt commandPrompt;

        // SensorManager sensors;
        // RingBuffer<int> recent_sensors;

    public:
        UI();
        void Update();
    };

    void start_ui();
}

#endif

/*
    //     void InitDisplay()
    //     {
    //         IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 0, 1);
    //         IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|   Table of Switches:   |\r\n", SWITCH_LOCATION + 1, 1);
    //         IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 2, 1);
    //         IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|  Switch  |   Status    |\r\n", SWITCH_LOCATION + 3, 1);
    //         IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 4, 1);
    //         for (int i = 0; i < SWITCH_COUNT; ++i)
    //         {
    //             // print borders and switch address so display only needs to update the status
    //             int location_y = SWITCH_LOCATION + 5 + i;
    //             IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|" MOVE_CURSOR "%d" MOVE_CURSOR "|" MOVE_CURSOR "|",
    //                          location_y, 1, location_y, 6, switches[i].address, location_y, 12, location_y, 25);
    //         }
    //         IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 5 + SWITCH_COUNT, 1);
    //     }

    //     void Display()
    //     {
    //         for (int i = 0; i < SWITCH_COUNT; ++i)
    //         {
    //             if (switches[i].updated)
    //             {
    //                 // print the char to exact location
    //                 SWITCH_STATE state = switches[i].ALIGNMENT;
    //                 if (state == SWITCH_STATE::STRAIGHT)
    //                 {
    //                     // colour green
    //                     IO_NS::Print(MOVE_CURSOR COLOR_GREEN "S", SWITCH_LOCATION + 5 + i, 20);
    //                 }
    //                 else
    //                 {
    //                     // colour red
    //                     IO_NS::Print(MOVE_CURSOR COLOR_RED "C", SWITCH_LOCATION + 5 + i, 20);
    //                 }
    //                 switches[i].updated = false;
    //             }
    //         }
    //     }

    */