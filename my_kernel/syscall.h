#ifndef _syscall_h_
#define _syscall_h_

extern "C" void _syscallHandler(int N, int PRIORITY, void (*function)());
extern "C" void _dummyHandler();

#endif // _syscall_h_