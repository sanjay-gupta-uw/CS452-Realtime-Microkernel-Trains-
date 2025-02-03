#ifndef _user_task_h_
#define _user_task_h_

#include "../clock.h"

extern Clock clock;

void Task1();
void TaskRegister();
void TaskQuery();
void TaskStressTest();

void PerformanceTask();

void SendTask();
void ReceiveTask();

#endif