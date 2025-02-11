#include "idle_task.h"
#include "syscall.h"

void IdleTask() {
    while (true) {
        YIELD();  // Yield to the scheduler, indicating idle status
        // Todo: Implement CPU time tracking here
    }
}