#include "../include/rps_client.h"
#include "../include/rps_server.h"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../rpi.h"
#include <cstring>

void RpsClient(GameMove *moves, int num_moves)
{
    int my_tid = MYTID();
    int server_tid = WHOIS("RPSServer");
    if (server_tid < 0)
    {
        uart_printf(CONSOLE, "Failed to find RPS Server\r\n");
        return;
    }
    uart_printf(CONSOLE, "RPS Server found by RPS Client with TID %d\r\n", my_tid);

    // Sign up for the game
    GameRequest req = {SIGNUP, ROCK}; // Initial move does not matter for signup
    char reply[20];
    int result = SEND(server_tid, (char *)&req, sizeof(req), reply, sizeof(reply));
    if (result < 0 || strcmp(reply, "PAIRED") != 0)
    {
        uart_printf(CONSOLE, "Failed to sign up or bad reply: %s\r\n", reply);
        return;
    }

    // Play the game according to the list of moves
    for (int i = 0; i < num_moves; i++)
    {
        uart_printf(CONSOLE, "move {%d / %d} = %d\r\n", i, num_moves, moves[i]);
        if (moves[i] == QUIT)
        {
            req.type = QUIT_GAME;
            SEND(server_tid, (char *)&req, sizeof(req), NULL, 0);
            uart_printf(CONSOLE, "Player TID %d quitting game.\r\n", my_tid);
            break;
        }
        else
        {
            // uart_printf(CONSOLE, "TID %d: Playing move %d\r\n", moves[i]);
            req.type = PLAY;
            req.move = static_cast<GameMove>(moves[i]);
            result = SEND(server_tid, (char *)&req, sizeof(req), reply, sizeof(reply));
            if (result < 0)
            {
                uart_printf(CONSOLE, "Failed to send play move\r\n");
                break;
            }
            if (strcmp(reply, "OTHER_QUIT") == 0)
            {
                uart_printf(CONSOLE, "Other player quit. Game over for TID %d.\r\n", my_tid);
                break;
            }
            else
            {
                uart_printf(CONSOLE, "client result for TID %d: %s\r\n", my_tid, reply);
            }
        }
    }
}
