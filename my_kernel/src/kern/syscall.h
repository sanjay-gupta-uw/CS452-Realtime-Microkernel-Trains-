#ifndef _syscall_h_
#define _syscall_h_

extern "C" int CREATE(int priority, void (*function)());
extern "C" int MYTID();
extern "C" int MYPARENTTID();
extern "C" void YIELD();
extern "C" void EXIT();
extern "C" int SEND(int tid, const char *msg, int msglen, char *reply, int replylen);
extern "C" int RECEIVE(int *tid, char *msg, int msglen);
extern "C" int REPLY(int tid, const char *reply, int replylen);

#endif // _syscall_h_