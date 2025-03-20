#ifndef _marklin_io_h_
#define _marklin_io_h_
#include <cstdint>
#include "../../containers/queue.h"
#include "marklin_structs.h"
#include "state_machine.h"

namespace MARKLIN_IO_SERVER
{
    struct MarklinRequest
    {
        bool isSingleByteCommand;
        uint8_t byte1;
        uint8_t byte2;
    };

#define UNDEFINED_CHAR '-'

    // REDEFINED QUEUE SIZE TO 32 -> change queue to accept size as a parameter?
    // #define RECEIVE_SIZE 32 // 32 chars/bytes
    class MarklinIOServer
    {
    public:
        MarklinIOServer();
        ~MarklinIOServer();

        // interface for the IO server

    private:
        uint32_t start_time;
        uint32_t end_time;
        int sequence_length;
        int count;
        bool canTransmit;
        int total_bytes_transmitted;

        int rx_notifier_tid;
        int tx_notifier_tid;
        int cts_notifier_high_tid;
        int cts_notifier_low_tid;

        Queue<unsigned char, 100> receive_buffer;
        Queue<uint8_t, 100> transmit_buffer; // this should be the bytes for commands

        Queue<int, 2> rx_waiting_tasks; // only sensor manager should be in here realistically

        void run();
        void spawnNotifiers();
        void write_to_uart();
        void handle_transmission();
    };

    enum class IO_REQUEST_TYPE
    {
        GETC,
        PUTC,
        SEND_CMD,
        RX_NOTIFIER,
        TX_NOTIFIER,
        CTS_NOTIFIER_HIGH,
        CTS_NOTIFIER_LOW,
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

    struct IO_REQUEST
    {
        IO_REQUEST_TYPE type;
        int data;
        MarklinRequest *request;
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
}

#endif // _marklin_io_h_