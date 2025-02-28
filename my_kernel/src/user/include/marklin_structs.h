#ifndef _marklin_structs_h_
#define _marklin_structs_h_

#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define OFF 0x20
#define REVERSE_CMD 0xF

enum class MARKLIN_REQUEST_TYPE
{
    SET_SWITCH,
    ACCELERATE_TRAIN,
    REVERSE_TRAIN,
    INVALID
};
struct MarklinRequest
{
    MARKLIN_REQUEST_TYPE type = MARKLIN_REQUEST_TYPE::INVALID;
    int id = -1;  // physcial id on the track
    int idx = -1; // index in the array
    unsigned char data = '\0';
};

#endif // _marklin_structs_h_