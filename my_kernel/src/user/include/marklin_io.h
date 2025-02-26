#ifndef _marklin_io_h_
#define _marklin_io_h_

#include "../../containers/queue.h"

namespace MARKLIN_IO_SERVER
{
#define UNDEFINED_CHAR '-'

    // state machine for CTS quirk
    enum class STATE_MACHINE
    {
        MARKLIN_READY,
        TRANSMITTING,
        MARKLIN_PROCESSING,
    };
    enum class PIN_STATE
    {
        LOW,
        HIGH
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
        int rx_notifier_tid;
        int tx_notifier_tid;
        int cts_notifier_tid;

        void run();
        void spawnNotifiers();
        void write_to_uart();
        void handle_transmission();

        Queue<unsigned char> receive_buffer;
        Queue<unsigned char> transmit_buffer; // this should be the bytes for commands

        Queue<int> rx_waiting_tasks;
        // pin states for CTS and TX
        PIN_STATE cts_state;
        // PIN_STATE tx_state;

        Queue<int> pending_tramissions;

        STATE_MACHINE tx_state_machine;
    };

    enum class IO_REQUEST_TYPE
    {
        GETC,
        PUTC,
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
        int channel;
        int data;
    };

    int Getc(int tid, int channel);
    int Putc(int tid, int channel, unsigned char ch);

    void notifier_rx();
    void notifier_tx();
    void notifier_cts();

// mapping of reply type to string
#define REPLY_TYPE_STR(type)                                                               \
    ((type == REPLY_TYPE::SUCCESS) ? "SUCCESS" : (type == REPLY_TYPE::FAILURE) ? "FAILURE" \
                                                                               : "UNIMPLEMENTED")

    void startMarklinIOServer();
}

#endif // _marklin_io_h_