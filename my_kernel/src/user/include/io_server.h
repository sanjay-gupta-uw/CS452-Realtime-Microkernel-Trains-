#ifndef _io_server_h_
#define _io_server_h_

#include "../../containers/queue.h"

namespace IO_SERVER
{
#define UNDEFINED_CHAR '-'
#define TX_HIGH 1 // buffer is not full
#define TX_LOW 0  // buffer is full/above threshold

    // REDEFINED QUEUE SIZE TO 32 -> change queue to accept size as a parameter?
    // #define RECEIVE_SIZE 32 // 32 chars/bytes
    class IOServer
    {
    public:
        IOServer();
        ~IOServer();

        // interface for the IO server

    private:
        int rtm_notifier_tid;
        int rx_notifier_tid;
        int tx_notifier_tid;

        void run();
        void spawnNotifiers();
        void write_to_uart();

        Queue<unsigned char> receive_buffer;
        Queue<unsigned char> transmit_buffer;

        Queue<int> rx_waiting_tasks;
    };

    enum class IO_REQUEST_TYPE
    {
        GETC,
        PUTC,
        RTM_NOTIFIER, // RECEIVE TIMEOUT NOTIFIER
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
        unsigned char ch;
    };

    int Getc(int tid, int channel);
    int Putc(int tid, int channel, unsigned char ch);

    void notifier_rxto();
    void notifier_tx();

// mapping of reply type to string
#define REPLY_TYPE_STR(type)                                                               \
    ((type == REPLY_TYPE::SUCCESS) ? "SUCCESS" : (type == REPLY_TYPE::FAILURE) ? "FAILURE" \
                                                                               : "UNIMPLEMENTED")

}

void startIOServer();

#endif // _io_server_h_