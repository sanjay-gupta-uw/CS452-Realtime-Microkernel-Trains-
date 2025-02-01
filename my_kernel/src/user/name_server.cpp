#include "name_server.h"
#include "../kern/syscall.h"
#include "../containers/ringbuffer.h"
#include "../containers/pqueue.h"
#include "../kern/kernel.h"
#include "../kern/task.h"
#include "../rpi.h"
#include <cstring>

#define MAX_NAME_ENTRIES 64
#define MAX_NAME_LENGTH 32

struct NameEntry {
    char name[MAX_NAME_LENGTH];
    int tid;
};

void NameServer() {
    uart_printf(CONSOLE, "Name Server running...\r\n");

    NameEntry names[MAX_NAME_ENTRIES];
    int name_count = 0;

    while (true) {
        int sender_tid;
        char msg[MAX_NAME_LENGTH + 2];

        RECEIVE(&sender_tid, msg, sizeof(msg));

        if (msg[0] == 'R') { // REGISTERAS request
            if (name_count < MAX_NAME_ENTRIES) {
                strncpy(names[name_count].name, msg + 1, MAX_NAME_LENGTH);
                names[name_count].tid = sender_tid;
                name_count++;

                if (REPLY(sender_tid, "OK", 2) < 0) {
                    uart_printf(CONSOLE, "NameServer: WARNING - Failed to REPLY to TID <%d>\r\n", sender_tid);
                }
            } else {
                if (REPLY(sender_tid, "FULL", 4) < 0) {
                    uart_printf(CONSOLE, "NameServer: WARNING - Failed to REPLY (FULL) to TID <%d>\r\n", sender_tid);
                }
            }
        } 
        else if (msg[0] == 'W') { // WHOIS request
            int found = 0;
            int found_tid = -1;

            for (int i = 0; i < name_count; i++) {
                if (strncmp(names[i].name, msg + 1, MAX_NAME_LENGTH) == 0) {
                    found_tid = names[i].tid;
                    found = 1;
                    break;
                }
            }

            if (!found) {
                found_tid = -1;
            }

            if (REPLY(sender_tid, (char*)&found_tid, sizeof(int)) < 0) {
                uart_printf(CONSOLE, "NameServer: WARNING - Failed to REPLY to TID <%d>\r\n", sender_tid);
            }
        }

        YIELD();
    }
}


int REGISTERAS(const char *name) {
    char msg[MAX_NAME_LENGTH + 2];
    char reply[4];

    msg[0] = 'R'; 
    strncpy(msg + 1, name, MAX_NAME_LENGTH);

    int ret = SEND(1, msg, sizeof(msg), reply, sizeof(reply));
    if (ret < 0) return -2;  // Error in SEND()

    return (strncmp(reply, "OK", 2) == 0) ? 0 : -1;
}

int WHOIS(const char *name) {
    char msg[MAX_NAME_LENGTH + 2];
    int tid = -1;

    msg[0] = 'W'; 
    strncpy(msg + 1, name, MAX_NAME_LENGTH);

    int ret = SEND(1, msg, sizeof(msg), (char*)&tid, sizeof(int));
    if (ret < 0) return -2;  // Error in SEND()

    return tid;
}

