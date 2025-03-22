#ifndef _user_task_h_
#define _user_task_h_

// extern "C" void Print(char *str);

struct ParamRequest
{
    int delay_interval;
    int num_delays;
};

void FirstUserTask();
void ClientTask();
void MarklinTask();
void IdleTask();
void ComputeIdleTask();

#endif