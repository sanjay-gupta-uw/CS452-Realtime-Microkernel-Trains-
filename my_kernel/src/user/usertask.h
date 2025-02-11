#ifndef _user_task_h_
#define _user_task_h_

#include "../clock.h"

extern Clock clock;

void Task1();
void TaskRegister();
void TaskQuery();
void TaskStressTest();

void PerformanceTask();

bool SendTask(int r_tid, int msglen, char *reply, int replylen);

void ReceiveTask();
void FirstUserTask_K3();

#endif