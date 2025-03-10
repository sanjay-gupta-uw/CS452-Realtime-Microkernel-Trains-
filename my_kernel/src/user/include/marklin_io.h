#ifndef _marklin_io_h_
#define _marklin_io_h_
#include <cstdint>
#include "../../containers/queue.h"
#include "marklin_structs.h"
#include "state_machine.h"

namespace MARKLIN_IO_SERVER
{
#define UNDEFINED_CHAR '-'

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
        unsigned int numSeenCommands;
        bool canTransmit;
        int active_command_size;
        int bytes_transmitted;
        int last_switch_addr;
        int total_bytes_transmitted;

        int rx_notifier_tid;
        int tx_notifier_tid;
        int cts_notifier_high_tid;
        int cts_notifier_low_tid;

        Queue<unsigned char, 100> receive_buffer;
        Queue<uint8_t, 100> transmit_buffer; // this should be the bytes for commands
        Queue<COMMAND, 50> cmd_buffer;       // 's', 't', 'r' for switch, train(accel), reverse

        Queue<int, 2> rx_waiting_tasks;
        // pin states for CTS and TX
        // PIN_STATE tx_state;
        TransmitMachine tx_state;

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
    void notifier_cts_high();
    void notifier_cts_low();

    // bool initialized;

// mapping of reply type to string
#define REPLY_TYPE_STR(type)                                                               \
    ((type == REPLY_TYPE::SUCCESS) ? "SUCCESS" : (type == REPLY_TYPE::FAILURE) ? "FAILURE" \
                                                                               : "UNIMPLEMENTED")

    void startMarklinIOServer();
}

#endif // _marklin_io_h_