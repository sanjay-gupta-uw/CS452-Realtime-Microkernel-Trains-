#ifndef _syscall_h_
#define _syscall_h_

extern "C" int CREATE(int priority, void (*function)());
extern "C" int MYTID();
extern "C" int MYPARENTTID();
extern "C" void YIELD();
extern "C" void EXIT();

#endif // _syscall_h_