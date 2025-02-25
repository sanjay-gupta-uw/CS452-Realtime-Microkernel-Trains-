#ifndef _io_server_h_
#define _io_server_h_

#include "../../containers/queue.h"

namespace IO_SERVER
{
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

// mapping of reply type to string
#define REPLY_TYPE_STR(type)                                                               \
    ((type == REPLY_TYPE::SUCCESS) ? "SUCCESS" : (type == REPLY_TYPE::FAILURE) ? "FAILURE" \
                                                                               : "UNIMPLEMENTED")

    struct IO_REQUEST
    {
        IO_REQUEST_TYPE type;
        int channel;
        unsigned char ch;
    };

    int Getc(int tid, int channel);
    int Putc(int tid, int channel, unsigned char ch);

    void notifierRTM();
    void notifierTX();
    void notifierRX();

}

void startIOServer();

#endif // _io_server_h_