#ifndef _marklin_io_h_
#define _marklin_io_h_
#include <cstdint>
#include "../../containers/queue.h"
#include "marklin_structs.h"
#include "state_machine.h"
#include <cstring>
#include <cstdlib>

// #include "../marklin/sensor.h"

namespace MARKLIN_IO_SERVER
{
#define NUM_BANKS 5
#define SENSORS_PER_BANK 16

    struct Sensor
    {
        char bank;
        uint8_t id;
        bool isTriggered;
        uint32_t trigger_tick;
    };
    extern Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK]; // Declaration

#define UNDEFINED_CHAR '-'
#define NUM_SWITCHES 22
#define MAX_RECENT_SENSORS 10

    struct MessengerUnit
    {
        int messenger_id;
        int train_num;
        int sensor_idx;
        bool sent_reply;
    };
    // REDEFINED QUEUE SIZE TO 32 -> change queue to accept size as a parameter?
    // #define RECEIVE_SIZE 32 // 32 chars/bytes
    class MarklinIOServer
    {
    public:
        MarklinIOServer();
        ~MarklinIOServer();

        // interface for the IO server

    private:
        int CLOCK_SERVER_TID;
        int SWITCH_ADDRS[NUM_SWITCHES];

        uint32_t start_time;
        uint32_t end_time;
        Queue<int, 32> sequence_length_buffer;
        int sequence_length;
        int count;
        int total_bytes_transmitted;
        bool canTransmit;
        bool isReceiving;
        int TOTAL_BYTES_TO_RECEIVE;
        int BYTES_RECEIVED;

        int rx_notifier_tid;
        int tx_notifier_tid;
        int cts_notifier_high_tid;
        int cts_notifier_low_tid;
        int switch_notifier_tid[NUM_SWITCHES];

        MessengerUnit sensor_messenger[NUM_TRAINS];

        Queue<unsigned char, 100> receive_buffer;
        Queue<uint8_t, 100> transmit_buffer; // this should be the bytes for commands

        Queue<int, 2> rx_waiting_tasks; // only sensor manager should be in here realistically

        void run();
        void spawnNotifiers();
        void write_to_uart();
        void handle_transmission();
        int isSwitchCommand(int addr);
        void PushSolenoidOffCommand();
    };

    enum class REPLY_TYPE
    {
        SUCCESS,
        FAILURE,
        UNIMPLEMENTED
    };

    struct IO_REPLY
    {
        REPLY_TYPE type;
        unsigned char ch;
    };

    struct MarklinRequest
    {
        bool isSingleByteCommand;
        uint8_t byte1;
        uint8_t byte2;
    };
    enum class IO_REQUEST_TYPE
    {
        GETC,
        PUTC,
        SEND_CMD,
        RX_NOTIFIER,
        TX_NOTIFIER,
        SWITCH_NOTIFIER,
        SENSOR_LISTENER,
        SENSOR_MESSENGER,
    };

    struct RECEIVED_BYTES_STRUCT
    {
        int count;
        unsigned char bytes[10];
    };
    struct SensorListener
    {
        int train_num;
        unsigned char sensor_name[4]; // 3 chars + null terminator
    };
    struct IO_REQUEST
    {
        IO_REQUEST_TYPE type;

        union Data
        {
            MarklinRequest *request;
            RECEIVED_BYTES_STRUCT *received_bytes;
            SensorListener *sensor_listener;

            Data()
            {
            }
            ~Data() {}
        } data;

        IO_REQUEST(MarklinRequest *request) : type(IO_REQUEST_TYPE::SEND_CMD)
        {
            data.request = request;
        }

        IO_REQUEST(RECEIVED_BYTES_STRUCT *ret_buff) : type(IO_REQUEST_TYPE::RX_NOTIFIER)
        {
            data.received_bytes = ret_buff;
        }
        IO_REQUEST(IO_REQUEST_TYPE type = IO_REQUEST_TYPE::TX_NOTIFIER) : type(type)
        {
        }
        IO_REQUEST(SensorListener *listener) : type(IO_REQUEST_TYPE::SENSOR_LISTENER)
        {
            // IO_NS::PrintTerminal("MarklinIO_server::SENSOR_LISTENER: Received request for sensor %s\r\n", listener->sensor_name);
            data.sensor_listener = listener;
            // IO_NS::PrintTerminal("MarklinIO_server::SENSOR_LISTENER: Received request for sensor %s\r\n", listener->sensor_name);
        }
    };

    int Getc(int tid);
    int Putc(int tid, unsigned char ch);
    int SendCmd(int tid, MarklinRequest *request);

    void notifier_rx();
    void notifier_tx();

    // bool initialized;

// mapping of reply type to string
#define REPLY_TYPE_STR(type)                                                               \
    ((type == REPLY_TYPE::SUCCESS) ? "SUCCESS" : (type == REPLY_TYPE::FAILURE) ? "FAILURE" \
                                                                               : "UNIMPLEMENTED")

    void startMarklinIOServer();
    void switchNotifier();
    void start_sensor_messenger();
}

#endif // _marklin_io_h_