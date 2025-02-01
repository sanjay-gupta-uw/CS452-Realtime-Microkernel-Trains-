#include "rps_server.h"
#include "name_server.h"
#include "../kern/syscall.h"
#include "../containers/ringbuffer.h"
#include "../containers/pqueue.h"
#include "../kern/kernel.h"
#include "../kern/task.h"
#include "../rpi.h"
#include <cstring>

void RPSServer() {
    REGISTERAS("RPS_SERVER");
    int players[2] = {-1, -1};
    char choices[2];
    
    while (true) {
        int sender_tid;
        char msg[10];
        RECEIVE(&sender_tid, msg, sizeof(msg));

        if (msg[0] == 'S') {  
            if (players[0] == -1) {
                players[0] = sender_tid;
            } else {
                players[1] = sender_tid;
                REPLY(players[0], "P", 1);
                REPLY(players[1], "P", 1);
            }
        } else if (msg[0] == 'P') {  
            if (players[0] == sender_tid) {
                choices[0] = msg[1];
            } else {
                choices[1] = msg[1];
            }
            
            if (choices[0] != 0 && choices[1] != 0) {
                char result = (choices[0] == choices[1]) ? 'D' :
                             ((choices[0] == 'R' && choices[1] == 'S') ||
                              (choices[0] == 'S' && choices[1] == 'P') ||
                              (choices[0] == 'P' && choices[1] == 'R')) ? 'W' : 'L';
                REPLY(players[0], &result, 1);
                REPLY(players[1], &result, 1);
                players[0] = players[1] = -1;
                choices[0] = choices[1] = 0;
            }
        }
    }
}
