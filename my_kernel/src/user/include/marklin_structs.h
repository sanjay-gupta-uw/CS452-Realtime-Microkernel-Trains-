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
    CALIBRATE,
    SENSOR_TRIGGER,
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

enum class TRAIN_COMMAND
{
    ACCELERATE,
    REVERSE,
    STOP,
    TICK,
};

enum class TrainMessageType
{
    PATH_MESSENGER,
    SENSOR_MESSENGER,
    TRAIN_TICKER,
};

struct SegmentStruct
{
    SensorStruct sensor; // next sensor to query for
    int segment_length;
};
struct TrainResponse
{
    TrainMessageType type;

    TRAIN_COMMAND command; // command to be executed, if any
    int speed;             // assume stop is only issued when next sensor is the destination
    SensorStruct sensor;
    int segment_length;
    int trigger_tick;
};

struct TrainMessage
{
    TrainMessageType type;

    union Data
    {
        SegmentStruct segment;
        TRAIN_COMMAND command;
        int speed;
        SensorStruct triggered_sensor;
        int trigger_tick;
        // Add a constructor to avoid potential undefined behavior
        Data() {}  // Default constructor
        ~Data() {} // Destructor
    } data;        // Named union member

    // default constructor
    TrainMessage(TrainMessageType type = TrainMessageType::TRAIN_TICKER)
        : type(type)
    {
    }

    // Constructor for PATH_MESSENGER
    TrainMessage(SensorStruct sensor, int segment_length, TRAIN_COMMAND command, int speed)
        : type(TrainMessageType::PATH_MESSENGER)
    {
        data.segment = {sensor, segment_length};
        data.command = command;
    }

    // Constructor for SENSOR_MESSENGER
    TrainMessage(int trigger_tick, SensorStruct sensor)
        : type(TrainMessageType::SENSOR_MESSENGER)
    {
        data.trigger_tick = trigger_tick;
        data.triggered_sensor = sensor;
    }
};

struct TrainQuery
{
    SensorStruct sensor;
    DIRECTION direction; // direction of the train
};

enum class RequestType
{
    CMD,
    GET_SEGMENT,
};
struct ConductorRequest
{
    RequestType requestType; // To track which union member is valid

    union Data
    {
        CMDRequest cmdRequest;
        int spawned_train_tid;

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
        if (src)
            data.cmdRequest = {command, id, requestData, src};
        if (src && dest)
            data.cmdRequest = {command, id, requestData, src, dest};
    }

    // Constructor for TrainQuery
    ConductorRequest(int train_tid)
        : requestType(RequestType::GET_SEGMENT)
    {
        data.spawned_train_tid = train_tid;
    }
};

#endif // _marklin_structs_h_