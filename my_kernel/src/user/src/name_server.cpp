#include "../include/name_server.h"
#include "../../include/syscall.h"
#include "../../rpi.h"
#include <cstring>
#include <cstdlib>

#define MAX_NAME_ENTRIES 64
#define MAX_NAME_LENGTH 32

struct NameEntry
{
    char name[MAX_NAME_LENGTH + 1]; // Ensure space for null-terminator
    int tid;
};

static int NAME_SERVER_TID = -1; // Global variable to store Name Server task ID

void NameServer()
{
    NAME_SERVER_TID = MYTID();

    NameEntry names[MAX_NAME_ENTRIES] = {};
    int name_count = 0;

    while (true)
    {
        int sender_tid;
        char msg[MAX_NAME_LENGTH + 2]; // Extra space for command and null-terminator

        // // uart_printf(CONSOLE, "NameServer: Waiting for message...\r\n");
        int received = RECEIVE(&sender_tid, msg, sizeof(msg) - 1);
        // // uart_printf(CONSOLE, "Received message from TID %d with content: %s\r\n", sender_tid, msg);

        if (received < 0)
        {
            // uart_printf(CONSOLE, "NameServer: Error receiving message.\r\n");
            continue;
        }
        msg[received] = '\0'; // Ensure null-termination

        char command = msg[0];
        const char *name = msg + 1;

        // // uart_printf(CONSOLE, "Processing command: %c for name: %s\r\n", command, name);

        if (command == 'R')
        { // REGISTERAS request
            int index = -1;
            for (int i = 0; i < name_count; ++i)
            {
                if (strcmp(names[i].name, name) == 0)
                {
                    index = i;
                    break;
                }
            }

            if (index == -1)
            { // New entry
                if (name_count >= MAX_NAME_ENTRIES)
                {
                    REPLY(sender_tid, "FULL", 5);
                    continue;
                }
                index = name_count++;
                strncpy(names[index].name, name, MAX_NAME_LENGTH);
                names[index].name[MAX_NAME_LENGTH] = '\0'; // Ensure null-termination
            }
            names[index].tid = sender_tid;
            // uart_printf(CONSOLE, "Registered name: %s with TID: %d\r`\n", name, sender_tid);
            REPLY(sender_tid, "OK", 3);
        }
        else if (command == 'W')
        { // WHOIS request
            int found_tid = -1;
            for (int i = 0; i < name_count; i++)
            {
                if (strcmp(names[i].name, name) == 0)
                {
                    found_tid = names[i].tid;
                    break;
                }
            }
            // // uart_printf(CONSOLE, "Lookup for name: %s found TID: %d\r\n", name, found_tid);
            // char *reply = reinterpret_cast<char *>(&found_tid);
            // // uart_printf(CONSOLE, "Reply: %s\r\n", found_tid);

            REPLY(sender_tid, reinterpret_cast<char *>(&found_tid), sizeof(found_tid));
        }
    }
}

int REGISTERAS(const char *name)
{
    // // uart_printf(CONSOLE, "REGISTERAS name: %s\r\n", name);
    if (NAME_SERVER_TID == -1)
    {
        return -3; // NameServer not started or unknown
    }

    // int my_tid = MYTID();

    char msg[MAX_NAME_LENGTH + 2] = {'R'};
    strncpy(msg + 1, name, MAX_NAME_LENGTH);
    msg[MAX_NAME_LENGTH + 1] = '\0'; // Ensure null-termination

    char reply[3];
    int reply_len = sizeof(reply);

    int ret = SEND(NAME_SERVER_TID, msg, strlen(msg) + 1, reply, reply_len);
    // // uart_printf(CONSOLE, "Registered TID {%d} with name {%s}\r\n", my_tid, name);
    if (ret < 0)
        return -2; // Error in SEND()
    return (strcmp(reply, "OK") == 0) ? 0 : -1;
}

int WHOIS(const char *name)
{
    if (NAME_SERVER_TID == -1)
    {
        return -3; // NameServer not started or unknown error
    }

    char msg[MAX_NAME_LENGTH + 2] = {'W'};
    strncpy(msg + 1, name, MAX_NAME_LENGTH);
    msg[MAX_NAME_LENGTH + 1] = '\0'; // Ensure null-termination

    int tid;
    int ret = SEND(NAME_SERVER_TID, msg, strlen(msg) + 1, reinterpret_cast<char *>(&tid), sizeof(tid));
    if (ret < 0)
        return -2; // Error in SEND()
    return tid;
}
