#ifndef _usercall_h_
#define _usercall_h_

inline int CreateTask(int priority, void (*function)())
{
    int tid = -1;
    asm volatile(
        "mov x1, %1\n"
        "mov x2, %2\n"
        "svc #0\n"     // Trigger SVC
        "mov %0, x0\n" // Move return value from x0 into `tid`
        : "=r"(tid)
        : "r"(priority), "r"(function)
        : "x1", "x2");
    return tid;
}

inline int MyTid()
{
    int tid = -1;
    asm volatile(
        "svc #1\n"     // Trigger SVC
        "mov %0, x0\n" // Move return value from x0 into `tid`
        : "=r"(tid)
        :
        : "x0");
    return tid;
}

inline int MyParentTid()
{
    int tid = -1;
    asm volatile(
        "svc #2\n"     // Trigger SVC
        "mov %0, x0\n" // Move return value from x0 into `tid`
        : "=r"(tid)
        :
        : "x0");
    return tid;
}

inline int Yield()
{
    int tid = -1;
    asm volatile(
        "svc #3\n"     // Trigger SVC
        "mov %0, x0\n" // Move return value from x0 into `tid`
        : "=r"(tid)
        :
        : "x0");
    return tid;
}

inline int Exit()
{
    int tid = -1;
    asm volatile(
        "svc #4\n"     // Trigger SVC
        "mov %0, x0\n" // Move return value from x0 into `tid`
        : "=r"(tid)
        :
        : "x0");
    return tid;
}
#endif