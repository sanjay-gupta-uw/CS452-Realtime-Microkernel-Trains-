#ifndef _marklin_structs_h_
#define _marklin_structs_h_

#include "uassert.h"
#include <cstdint>
#include "../../util.h"
#define STRAIGHT_CMD 0x21
#define CURVED_CMD 0x22
#define SOLENOID_OFF_CMD 0x20
#define REVERSE_CMD 0xF
#define NUM_TRAINS 5

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
    STOP_ALL,
    INVALID,
};

struct CMDRequest
{
    COMMAND command;
    int id;
    int data;
    char *src;
    char *dest;
    int offset;
};

struct SensorStruct
{
    char bank;
    int id;
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
        // IO_NS::PrintTerminal("TrainMessage::TrainMessage CONSTRUCTOR -- %d\r\n", type);
    }

    // Constructor for SegmentNotification
    TrainMessage(char bank, int id, int segment_length)
        : type(TrainMessageType::PATH_MESSENGER)
    {
        data.segment = {SensorStruct{bank, id}, segment_length};
        IO_NS::PrintTerminal("TrainMessage::SegmentNotification CONSTRUCTOR -- %c%d, %d\r\n", bank, id, segment_length);
    }

    // Constructor for TrainCommandNotification
    TrainMessage(TRAIN_COMMAND command, int speed)
        : type(TrainMessageType::TRAIN_COMMAND)
    {
        data.train_command = {command, speed};
        // IO_NS::PrintTerminal("TrainMessage::TrainCommandNotification CONSTRUCTOR -- %d, %d\r\n", command, speed);
    }

    // Constructor for SensorPingNotification
    TrainMessage(int trigger_tick, char bank, int id)
        : type(TrainMessageType::SENSOR_MESSENGER)
    {
        data.sensor_ping = {SensorStruct{bank, id}, trigger_tick};
        // IO_NS::PrintTerminal("TrainMessage::SensorPingNotification CONSTRUCTOR -- %d, %c%d\r\n", trigger_tick, bank, id);
    }
};

struct TrainQuery
{
    SensorStruct sensor;
    DIRECTION direction; // direction of the train
};

enum class RequestType
{
    CMD,            // CONSOLE COMMANDS
    GET_SEGMENT,    // GET NEXT SEGMENT
    GET_CMD,        // GET TRAIN COMMAND
    GET_SENSOR,     // GET SENSOR (sensor messenger to marklin)
    SENSOR_TRIGGER, // NOTICE OF SENSOR TRIGGER
    TICK,
};
struct SensorTriggerResponse
{
    bool is_triggered;
    uint32_t trigger_tick;
    int sensor_idx;
    int train_num;
};
struct ConductorRequest
{
    RequestType requestType; // To track which union member is valid

    union Data
    {
        CMDRequest cmdRequest;
        int train_num;
        SensorTriggerResponse sensor_trigger_response;

        // Add a constructor to avoid potential undefined behavior
        Data() {}  // Default constructor
        ~Data() {} // Destructor
    } data;        // Named union member

    // default constructor
    ConductorRequest()
    {
    }

    ConductorRequest(RequestType requestType)
        : requestType(requestType)
    {
        // IO_NS::PrintTerminal("ConductorRequest::ConductorRequest CONSTRUCTOR -- %d\r\n", requestType);
    }

    // Constructor for CMDRequest
    ConductorRequest(COMMAND command, int id, int requestData, char *src = nullptr, char *dest = nullptr, int offset = 0)
        : requestType(RequestType::CMD)
    {
        data.cmdRequest = {command, id, requestData};
        if (src)
            data.cmdRequest = {command, id, requestData, src};
        if (src && dest)
            data.cmdRequest = {command, id, requestData, src, dest};
        if (src && dest && offset)
            data.cmdRequest = {command, id, requestData, src, dest, offset};
    }

    ConductorRequest(int train_num)
        : requestType(RequestType::GET_CMD)
    {
        data.train_num = train_num;
    }

    ConductorRequest(int train_num, RequestType requestType)
        : requestType(requestType)
    {
        data.train_num = train_num;
        // IO_NS::PrintTerminal("ConductorRequest::ConductorRequest CONSTRUCTOR -- %d\r\n", requestType);
    }
    ConductorRequest(SensorTriggerResponse *sensor_trigger_response)
        : requestType(RequestType::SENSOR_TRIGGER)
    {
        data.sensor_trigger_response = *sensor_trigger_response;
        // IO_NS::PrintTerminal("ConductorRequest::ConductorRequest CONSTRUCTOR -- %d\r\n", requestType);
    }
};

#endif // _marklin_structs_h_