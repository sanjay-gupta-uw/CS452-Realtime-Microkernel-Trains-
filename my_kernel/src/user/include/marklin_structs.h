#ifndef _marklin_structs_h_
#define _marklin_structs_h_

#include "uassert.h"
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
    STOP_ALL,
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
    BANKS bank;
    int id;
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
    TICK,
    STOP_ALL,
    SENSOR_TRIGGER,
};

struct TrainResponse
{
    TRAIN_COMMAND command; // command to be executed, if any
    int speed;             // assume stop is only issued when next sensor is the destination
    SensorStruct sensor;
};

enum class TrainMessageType
{
    TRAIN_COMMAND,
    PATH_MESSENGER,
    SENSOR_MESSENGER,
    TRAIN_TICKER,
};

// USE THIS FOR SENDING SENSOR HIT BACK TO TRAIN
struct SensorPingNotification
{
    SensorStruct triggered_sensor;
    int trigger_tick;
};
struct SegmentNotification
{
    SensorStruct sensor;
    int segment_length;
};

struct TrainCommandNotification
{
    TRAIN_COMMAND command;
    int speed;
};

struct TrainMessage
{
    TrainMessageType type;

    union Data
    {
        SegmentNotification segment;
        TrainCommandNotification train_command;
        SensorPingNotification sensor_ping;

        // Add a constructor to avoid potential undefined behavior
        Data() {}  // Default constructor
        ~Data() {} // Destructor
    } data;        // Named union member

    // default constructor
    TrainMessage(TrainMessageType type = TrainMessageType::TRAIN_TICKER)
        : type(type)
    {
        IO_NS::PrintTerminal("TrainMessage::TrainMessage CONSTRUCTOR -- %d\r\n", type);
    }

    // Constructor for SegmentNotification
    TrainMessage(char bank, int id, int segment_length)
        : type(TrainMessageType::PATH_MESSENGER)
    {
        //data.segment = {SensorStruct{bank, id}, segment_length};
        IO_NS::PrintTerminal("TrainMessage::SegmentNotification CONSTRUCTOR -- %c%d, %d\r\n", bank, id, segment_length);
    }

    // Constructor for TrainCommandNotification
    TrainMessage(TRAIN_COMMAND command, int speed)
        : type(TrainMessageType::TRAIN_COMMAND)
    {
        data.train_command = {command, speed};
        IO_NS::PrintTerminal("TrainMessage::TrainCommandNotification CONSTRUCTOR -- %d, %d\r\n", command, speed);
    }

    // Constructor for SensorPingNotification
    TrainMessage(int trigger_tick, char bank, int id)
        : type(TrainMessageType::SENSOR_MESSENGER)
    {
        //data.sensor_ping = {SensorStruct{bank, id}, trigger_tick};
        IO_NS::PrintTerminal("TrainMessage::SensorPingNotification CONSTRUCTOR -- %d, %c%d\r\n", trigger_tick, bank, id);
    }
};

enum class RequestType
{
    CMD,         // CONSOLE COMMANDS
    TRAIN,
    GET_SEGMENT, // GET NEXT SEGMENT
    GET_CMD,     // GET TRAIN COMMAND
};
struct ConductorRequest
{
    RequestType requestType; // To track which union member is valid

    union Data
    {
        CMDRequest cmdRequest;
        TrainQuery trainQuery;
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
    ConductorRequest(SensorStruct sensor, DIRECTION direction)
        : requestType(RequestType::TRAIN)
    {
        data.trainQuery = {sensor, direction};
    }

    // Constructor for TrainQuery
    ConductorRequest(int train_tid)
        : requestType(RequestType::GET_SEGMENT)
    {
        data.spawned_train_tid = train_tid;
    }

    ConductorRequest(int train_tid, int train_num)
        : requestType(RequestType::GET_CMD)
    {
        data.spawned_train_tid = train_tid;
    }
};

#endif // _marklin_structs_h_