#include "../include/name_server.h"
#include "../../include/syscall.h"
#include "../../rpi.h"
#include <cstring>
#include <cstdlib>
#include "../include/io.h"
#include "../include/uassert.h"
#include "../../shared_constants.h"

#define MAX_NAME_ENTRIES 64
#define MAX_NAME_LENGTH 32

enum class REPLY_TYPE
{
    SUCCESS,
    FAILURE,
};
struct ReplyType
{
    REPLY_TYPE type;
    int index;
    int tid;
};

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
    ReplyType reply;

    while (true)
    {

        reply = {REPLY_TYPE::FAILURE, -1, -1};
        char msg[MAX_NAME_LENGTH + 2]; // Extra space for command and null-terminator
        for (int i = 0; i < MAX_NAME_LENGTH + 2; ++i)
        {
            msg[i] = '\0';
        }

        // (CONSOLE, "NameServer: Waiting for message...\r\n");
        int sender_tid;
        int received = RECEIVE(&sender_tid, msg, sizeof(msg) - 1);
        if (received < 0 || received > MAX_NAME_LENGTH + 1)
        {
            continue;
        }
        msg[received] = '\0'; // Ensure null-termination

        char command = msg[0];
        const char *name = msg + 1;

        // uart_printf(CONSOLE, RESTORE_CURSOR "Processing command: %c for name: %s\r\n" SAVE_CURSOR, command, msg + 1);
        switch (command)
        {
        case 'R':
        {
            ReplyType reply;
            reply.index = -1;
            reply.tid = -1;

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
                    REPLY(sender_tid, (const char *)&reply, sizeof(reply));
                    continue;
                }
                index = name_count++;
                strncpy(names[index].name, name, received - 1);
                // strncpy(names[index].name, name, MAX_NAME_LENGTH);
                // names[index].name[MAX_NAME_LENGTH] = '\0'; // Ensure null-termination
            }
            names[index].tid = sender_tid;
            reply = {REPLY_TYPE::SUCCESS, index, sender_tid};
        }
        break;
        case 'W':
        {
            // WHOIS request
            int found_tid = -1;
            int found_index = -1;
            for (int i = 0; i < name_count; i++)
            {
                if (strcmp(names[i].name, name) == 0)
                {
                    found_tid = names[i].tid;
                    found_index = i;
                    break;
                }
            }
            reply = {REPLY_TYPE::SUCCESS, found_index, found_tid};
        }
        default:
            break;
        }

        // uart_printf(CONSOLE, RESTORE_CURSOR "Replying to sender_tid: %d with tid: %d\r\n" SAVE_CURSOR, sender_tid, reply.tid);
        REPLY(sender_tid, (const char *)&reply, sizeof(reply));
    }
}

int REGISTERAS(const char *name)
{
    if (NAME_SERVER_TID == -1)
    {
        return -3; // NameServer not started or unknown
    }

    // int my_tid = MYTID();

    char msg[MAX_NAME_LENGTH + 2] = {'R'};
    strncpy(msg + 1, name, MAX_NAME_LENGTH);
    msg[MAX_NAME_LENGTH + 1] = '\0'; // Ensure null-termination

    ReplyType reply;

    int ret = SEND(NAME_SERVER_TID, msg, strlen(msg) + 1, reinterpret_cast<char *>(&reply), sizeof(reply));

    if (reply.type == REPLY_TYPE::SUCCESS)
    {
        // uart_printf(CONSOLE, RESTORE_CURSOR "%s: TID %d\r\n" SAVE_CURSOR, TID_NAMES_LOCATION + 1 + reply.index, name, reply.tid);
        // IO_NS::Print(MOVE_CURSOR "%s: TID %d\r\n", TID_NAMES_LOCATION + 1 + reply.index, TID_COL, name, reply.tid);
    }
    return reply.tid;
}

int WHOIS(const char *name)
{
    // uart_printf(CONSOLE, RESTORE_CURSOR "WHOIS: {%s}\r\n" SAVE_CURSOR, name);
    if (NAME_SERVER_TID == -1)
    {
        return -3; // NameServer not started or unknown error
    }

    char msg[MAX_NAME_LENGTH + 2] = {'W'};
    strncpy(msg + 1, name, MAX_NAME_LENGTH);
    msg[MAX_NAME_LENGTH + 1] = '\0'; // Ensure null-termination

    ReplyType reply;
    // uart_printf(CONSOLE, RESTORE_CURSOR "WHOIS: {%s} -- Sending to NameServer\r\n" SAVE_CURSOR, name);
    int ret = SEND(NAME_SERVER_TID, msg, strlen(msg) + 1, reinterpret_cast<char *>(&reply), sizeof(reply));
    // uart_printf(CONSOLE, RESTORE_CURSOR "WHOIS: {%s} -- Received from NameServer\r\n" SAVE_CURSOR, name);

    if (reply.type == REPLY_TYPE::SUCCESS)
    {
    }
    return reply.tid;
}
