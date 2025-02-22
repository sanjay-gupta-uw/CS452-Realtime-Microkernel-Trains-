#ifndef _io_server_h_
#define _io_server_h_

class IOServer
{
public:
    IOServer();
    ~IOServer();

    // interface for the IO server
    int Getc(int tid, int channel);
    int Putc(int tid, int channel, unsigned char ch);

private:
    void run();
    void notifyRX();
    void notifyTX();
};

#endif // _io_server_h_