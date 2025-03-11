#ifndef _marklin_structs_h_
#define _marklin_structs_h_

#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define OFF 0x20
#define REVERSE_CMD 0xF

enum class COMMAND
{
    READ_SENSOR,
    SET_SWITCH,
    ACCELERATE_TRAIN,
    REVERSE_STOP_TRAIN,
    REVERSE_TRAIN,
    SOLENOID_OFF,
    INVALID,
};
struct MarklinRequest
{
    COMMAND type = COMMAND::INVALID;
    int id = -1;  // physcial id on the track
    int idx = -1; // index in the array
    unsigned char data = '\0';
};

#endif // _marklin_structs_h_