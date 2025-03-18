#ifndef _marklin_structs_h_
#define _marklin_structs_h_

#include <cstdint>

#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define SOLENOID_OFF_CMD 0x20
#define REVERSE_CMD 0xF

enum class COMMAND
{
    SET_SWITCH,
    ACCELERATE_TRAIN,
    REVERSE_TRAIN,
    SPAWN_TRAIN,
    GOTO,
    NAVIGATE_LOOP, // NEED TO IMPLEMENT IN COMMAND.cpp
    INVALID,
};

struct ConductorRequest
{
    COMMAND command;
    int id; // physical ID of train, or index of switch
    int data;
};

struct MarklinRequest
{
    bool isSingleByteCommand;
    uint8_t byte1;
    uint8_t byte2;
};

#endif // _marklin_structs_h_