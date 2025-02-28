#ifndef _marklin_io_h_
#define _marklin_io_h_
#include <cstdint>
#include "../../containers/queue.h"
#include "marklin_structs.h"

namespace MARKLIN_IO_SERVER
{
#define UNDEFINED_CHAR '-'

    // state machine for CTS quirk
    enum class STATES
    {
        BYTE_CHOSEN,
        TX_HIGH,
        CTS_HIGH,
        CTS_LOW,
        CTS_HIGH2,
        TX_HIGH2,
        TOTAL_STATES
    };

#define NUM_STATES (int)STATES::TOTAL_STATES

    enum class STATUS
    {
        ACTIVE,
        INACTIVE
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
        // int rx_notifier_tid;
        // int tx_notifier_tid;
        // int cts_notifier_tid;

        void run();
        void spawnNotifiers();
        void write_to_uart();
        void handle_transmission();

        Queue<unsigned char, 32> receive_buffer;
        Queue<uint8_t, 32> transmit_buffer;  // this should be the bytes for commands
        Queue<unsigned char, 32> cmd_buffer; // 's', 't', 'r' for switch, train(accel), reverse

        Queue<int, 2> rx_waiting_tasks;
        // pin states for CTS and TX
        // PIN_STATE tx_state;
    };

    enum class IO_REQUEST_TYPE
    {
        GETC,
        PUTC,
        SEND_CMD,
        RX_NOTIFIER,
        TX_NOTIFIER,
        CTS_NOTIFIER
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

    // void notifier_rx();
    // void notifier_tx();
    // void notifier_cts();

// mapping of reply type to string
#define REPLY_TYPE_STR(type)                                                               \
    ((type == REPLY_TYPE::SUCCESS) ? "SUCCESS" : (type == REPLY_TYPE::FAILURE) ? "FAILURE" \
                                                                               : "UNIMPLEMENTED")

    void startMarklinIOServer();
}

#endif // _marklin_io_h_