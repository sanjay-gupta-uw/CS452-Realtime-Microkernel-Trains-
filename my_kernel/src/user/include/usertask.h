#ifndef _user_task_h_
#define _user_task_h_

struct ParamRequest
{
    int delay_interval;
    int num_delays;
};

void FirstUserTask();
void ClientTask();
void MarklinTask();
void IdleTask();
#endif