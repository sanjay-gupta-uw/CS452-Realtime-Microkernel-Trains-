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

struct CMDRequest
{
    COMMAND command;
    int id;
    int data;
    char *src;
    char *dest;
};

enum class RequestType
{
    CMD,
    TRAIN
};

enum class BANKS
{
    A,
    B,
    C,
    D,
    E,
};
struct SensorStruct
{
    BANKS bank; // A, B, C, D, E
    int id;     // 1-16
};

enum class DIRECTION
{
    FORWARD,
    REVERSE,
};
struct TrainQuery
{
    SensorStruct sensor;
    DIRECTION direction; // direction of the train
};

enum class TRAIN_COMMAND
{
    ACCELERATE,
    REVERSE,
    STOP,
};
struct TrainResponse
{
    TRAIN_COMMAND command; // command to be executed, if any
    int speed;             // assume stop is only issued when next sensor is the destination
    SensorStruct sensor;
};
struct ConductorRequest
{
    RequestType requestType; // To track which union member is valid

    union Data
    {
        CMDRequest cmdRequest;
        TrainQuery trainQuery;

        // Add a constructor to avoid potential undefined behavior
        Data() {}  // Default constructor
        ~Data() {} // Destructor
    } data;        // Named union member

    // default constructor
    ConductorRequest() {}

    // Constructor for CMDRequest
    ConductorRequest(COMMAND command, int id, int requestData, char *src = nullptr, char *dest = nullptr)
        : requestType(RequestType::CMD)
    {
        data.cmdRequest = {command, id, requestData};
        if(src) data.cmdRequest = {command, id, requestData, src};
        if(src && dest) data.cmdRequest = {command, id, requestData, src, dest};
    }

    // Constructor for TrainQuery
    ConductorRequest(SensorStruct sensor, DIRECTION direction)
        : requestType(RequestType::TRAIN)
    {
        data.trainQuery = {sensor, direction};
    }
};

#endif // _marklin_structs_h_