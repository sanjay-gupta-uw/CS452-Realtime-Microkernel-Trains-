#ifndef _syscall_h_
#define _syscall_h_
#include <cstdint>

extern "C" int CREATE(int priority, void (*function)());
extern "C" int MYTID();
extern "C" int MYPARENTTID();
extern "C" void YIELD();
extern "C" void EXIT();
extern "C" int SEND(int tid, const char *msg, int msglen, char *reply, int replylen);
extern "C" int RECEIVE(int *tid, char *msg, int msglen);
extern "C" int REPLY(int tid, const char *reply, int replylen);
extern "C" void ENABLE_ICACHE();
extern "C" void ENABLE_DCACHE();
extern "C" void ENABLE_BCACHE();
extern "C" int AWAITEVENT(int eventType);
extern "C" uint32_t GETIDLEPERCENT();
extern "C" void ABORT(const char *msg, int msglen, int line, const char *file);

// extern "C" void GENERATE_SGI();
#endif // _syscall_h_