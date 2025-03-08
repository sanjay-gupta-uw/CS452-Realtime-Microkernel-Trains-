#include "../include/rps_client.h"
#include "../include/rps_server.h"
#include "../include/name_server.h"
#include "../../include/syscall.h"
#include "../../rpi.h"
#include <cstring>
#include "../include/uassert.h"

void RpsClient(GameMove *moves, int num_moves)
{
    int my_tid = MYTID();
    int server_tid = WHOIS("RPSServer");
    uassert(server_tid >= 0 && "Failed to find RPS Server");
    // Sign up for the game
    GameRequest req = {SIGNUP, ROCK}; // Initial move does not matter for signup
    char reply[20];
    int result = SEND(server_tid, (char *)&req, sizeof(req), reply, sizeof(reply));
    if (result < 0 || strcmp(reply, "PAIRED") != 0)
    {
        //  (CONSOLE, "Failed to sign up or bad reply: %s\r\n", reply);
        return;
    }

    // Play the game according to the list of moves
    for (int i = 0; i < num_moves; i++)
    {
        if (moves[i] == QUIT)
        {
            req.type = QUIT_GAME;
            SEND(server_tid, (char *)&req, sizeof(req), NULL, 0);
            break;
        }
        else
        {
            req.type = PLAY;
            req.move = static_cast<GameMove>(moves[i]);
            result = SEND(server_tid, (char *)&req, sizeof(req), reply, sizeof(reply));
            if (result < 0)
            {
                break;
            }
            if (strcmp(reply, "OTHER_QUIT") == 0)
            {
                break;
            }
            else
            {
                // (CONSOLE, "client result for TID %d: %s\r\n", my_tid, reply);
            }
        }
    }
}
