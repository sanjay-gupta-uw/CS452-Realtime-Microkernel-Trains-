#include "../include/login.h"
#include "../include/io.h"
#include "../include/ui.h"
#include "../../include/syscall.h"
#include "../include/uassert.h"
#include "../../shared_constants.h"

void login_shell()
{
    // uassert(false && "FORCED PANIC -- LOGIN -- REMOVE THIS LINE");
    IO_NS::PrintTerminal("Login Shell started\r\n");
    int sender_tid;
    char buffer[100];
    // int retval = RECEIVE(&sender_tid, buffer, sizeof(buffer));
    IO_NS::PrintTerminal("Login Shell received request\r\n");
    while (true)
    {
    }
    // uassert(retval == sizeof(buffer));
    // IO_NS::PrintTerminal("Login Shell received request: %s\r\n", buffer);
    // REPLY(sender_tid, nullptr, 0);

    int uiTaskTid = CREATE(PRIORITY::CORE, UI_NS::start_ui); // Start the UI Task
    uassert(uiTaskTid > 0 && "Error starting UI Task");
    IO_NS::PrintTerminal("UI Task started with TID %d\r\n", uiTaskTid);
}