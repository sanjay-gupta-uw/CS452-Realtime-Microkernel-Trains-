#ifndef _marklin_structs_h_
#define _marklin_structs_h_

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
    int *data = nullptr;
};

#endif // _marklin_structs_h_