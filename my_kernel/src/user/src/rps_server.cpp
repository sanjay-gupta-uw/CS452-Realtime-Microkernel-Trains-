#include "../include/rps_server.h"
#include "../include/rps_client.h"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../rpi.h"
#include <cstring>
#include "../../containers/queue.h"

#define MAX_PLAYERS QUEUE_MAX_SIZE

struct Player
{
    int tid;
    int choice = -1;
    bool active = false;
    bool inGame = false;
    Player *opponent_player = nullptr;
    bool markDead = true;
};

Queue<Player *> freePlayers;
Queue<Player *> readyQueue;
// Queue<Player *> activePlayers;
Player players[MAX_PLAYERS];
int player_count = 0;

void matchPlayers();
void handlePlay(int index, int choice);
void resolveGame(int player_index);

void RpsServer()
{
    uart_printf(CONSOLE, "RPS Server running...\r\n");
    REGISTERAS("RPSServer");

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        players[i] = {-1, -1, false, false};
        freePlayers.Push(&players[i]);
    }

    while (true)
    {
        uart_printf(CONSOLE, "RPS Server waiting for requests...\r\n");
        int sender_tid;
        GameRequest req;
        int sendLen = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
        // uart_printf(CONSOLE, "RPS Server received request from TID %d of length %d\r\n", sender_tid, sendLen);
        // uart_printf(CONSOLE, "Request type: %d, move: %d\r\n", req.type, req.move);

        int index = -1;
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (players[i].tid == sender_tid)
            {
                index = i;
                break;
            }
        }

        switch (req.type)
        {
        case SIGNUP:
            if (index == -1 && !freePlayers.IsEmpty())
            {
                // player: tid, choice, active, inGame, opponent_player, markDead
                Player *player = nullptr;
                freePlayers.Pop(&player);
                *player = {sender_tid, -1, true, false, nullptr, false};
                player_count++;
                uart_printf(CONSOLE, "Player %d signed up.\r\n", player->tid);

                readyQueue.Push(player);
                matchPlayers(); // Check if two players can be paired
            }
            else
            {
                REPLY(sender_tid, "FULL", 5); // Server full
            }
            break;
        case PLAY:
            if (index != -1 && players[index].active && players[index].inGame)
            {
                uart_printf(CONSOLE, "PLEASE PRESS SPACE TO CONSUME MOVE\r\n");
                // wait for space key to handle input
                while (uart_getc(CONSOLE) != ' ')
                    ;
                clear_screen(CONSOLE);
                handlePlay(index, req.move);
            }
            break;
        case QUIT_GAME:
            if (index != -1)
            {
                Player *quit_player = &players[index];
                Player *p2 = quit_player->opponent_player;

                quit_player->markDead = true;
                resolveGame(index); // Resolve game if mid-match
                uart_printf(CONSOLE, "Player %d quit.\r\n", quit_player->tid);
                // matchPlayers();     // Attempt to match remaining players
            }
            break;
        }
        YIELD(); // Yield CPU to other tasks
    }
}

const char *getMoveName(int move)
{
    switch (move)
    {
    case ROCK:
        return "Rock";
    case PAPER:
        return "Paper";
    case SCISSORS:
        return "Scissors";
    default:
        return "Unknown";
    }
}

char getResultDescription(int result)
{
    switch (result)
    {
    case 0:
        return 'D';
    case 1:
        return 'W';
    case -1:
        return 'L';
    default:
        return 'U';
    }
}

void matchPlayers()
{
    while (!readyQueue.IsEmpty())
    {
        Player *p1, *p2 = nullptr;
        readyQueue.Pop(&p1);
        readyQueue.Pop(&p2);

        if (p2 == nullptr)
        {
            readyQueue.Push(p1);
            break;
        }
        else if (!p1->inGame && !p2->inGame && p1->tid != p2->tid && p1->active && p2->active)
        {
            p1->inGame = p2->inGame = true;
            p1->opponent_player = p2;
            p2->opponent_player = p1;

            REPLY(p1->tid, "PAIRED", 7);
            REPLY(p2->tid, "PAIRED", 7);

            // activePlayers.Push(p1);
            uart_printf(CONSOLE, "Players %d and %d are paired for a game.\r\n", p1->tid, p2->tid);
        }
        else
        {
            freePlayers.Push(p1);
            freePlayers.Push(p2);
            break;
        }
    }
}

void handlePlay(int index, int choice)
{
    Player *player = &players[index];
    if (player->opponent_player != nullptr && player->active && player->inGame)
    {
        player->choice = choice;
        uart_printf(CONSOLE, "Player %d played %s.\r\n", players[index].tid, getMoveName(choice));

        if (player->opponent_player->choice != -1)
        {
            resolveGame(index);
        }
    }
    else
    {
        uart_printf(CONSOLE, "Potential logic error in HandlePlay function.\r\n");
    }
}

static void reclaimPlayer(int index)
{
    Player *p1 = &players[index];
    Player *p2 = p1->opponent_player;
    uart_printf(CONSOLE, "Reclaiming partner player %d .\r\n", p2->tid);

    p1->choice = p2->choice = -1;
    p1->inGame = p2->inGame = false;
    p1->active = p2->active = false;
    p1->markDead = p2->markDead = true;
    p1->opponent_player = nullptr;
    p2->opponent_player = nullptr;

    freePlayers.Push(p1);
    freePlayers.Push(p2);
}

void resolveGame(int player_index)
{
    Player *p1 = &players[player_index];
    Player *p2 = p1->opponent_player;

    if (p1->markDead)
    {
        uart_printf(CONSOLE, "P1 marked dead, Game unresolved for Player %d: Waiting for an opponent.\r\n", p1->tid);
        reclaimPlayer(player_index);
        return;
    }

    char result1Desc = getResultDescription(p1->choice == p2->choice ? 0 : (p1->choice + 1) % 3 == p2->choice ? -1
                                                                                                              : 1);
    char result2Desc = getResultDescription(p1->choice == p2->choice ? 0 : (p1->choice + 1) % 3 == p2->choice ? 1
                                                                                                              : -1);

    const char *winDesc = "Win";
    const char *drawDesc = "Draw";
    const char *loseDesc = "Lose";
    const char *unkownDesc = "Unknown";

    char results[2] = {result1Desc, result2Desc};
    const char *desc[2];
    for (int i = 0; i < 2; ++i)
    {
        switch (results[i])
        {
        case 'W':
            desc[i] = winDesc;
            break;
        case 'D':
            desc[i] = drawDesc;
            break;
        case 'L':
            desc[i] = loseDesc;
            break;
        default:
            desc[i] = unkownDesc;
            break;
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        uart_printf(CONSOLE, "Desc %d: %s\r\n", i, desc[i]);
    }

    uart_printf(CONSOLE, "Game result: Player %d %s\r\n", p1->tid, desc[0]);
    uart_printf(CONSOLE, "Game result: Player %d %s\r\n", p2->tid, desc[1]);

    REPLY(p1->tid, desc[0], strlen(desc[0]) + 1);
    REPLY(p2->tid, desc[1], strlen(desc[1]) + 1);

    p1->choice = p2->choice = -1;
    // p1->inGame = p2->inGame = false;
}
