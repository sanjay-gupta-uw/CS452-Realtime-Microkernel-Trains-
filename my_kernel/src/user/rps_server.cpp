#include "rps_server.h"
#include "name_server.h"
#include "../kern/syscall.h"
#include "../rpi.h"
#include <cstring>

#define MAX_PLAYERS 10

enum RequestType {
    SIGNUP,
    PLAY,
    QUIT
};

enum GameMove {
    ROCK,
    PAPER,
    SCISSORS
};

struct GameRequest {
    RequestType type;  
    GameMove move;  
};

struct Player {
    int tid;
    int choice;
    bool active;
};

Player players[MAX_PLAYERS];
int player_count = 0;

void handlePlay(int index, int choice);
void resolveGame(int p1_index, int p2_index);

void RpsServer() {
    uart_printf(CONSOLE, "RPS Server running...\r\n");

    // Register server name with the Name Server
    REGISTERAS("RPSServer");

    while (true) {
        int sender_tid;
        GameRequest req;
        RECEIVE(&sender_tid, (char*)&req, sizeof(req));

        int index = -1;
        for (int i = 0; i < player_count; ++i) {
            if (players[i].tid == sender_tid) {
                index = i;
                break;
            }
        }

        switch (req.type) {
            case SIGNUP:
                if (index == -1 && player_count < MAX_PLAYERS) {
                    players[player_count] = {sender_tid, -1, true};
                    player_count++;
                    uart_printf(CONSOLE, "Player %d signed up.\r\n", sender_tid);
                    REPLY(sender_tid, "OK", 3);  // Confirm signup
                } else {
                    REPLY(sender_tid, "FULL", 5);  // Server full
                }
                break;
            case PLAY:
                if (index != -1 && players[index].active) {
                    handlePlay(index, req.move);
                }
                break;
            case QUIT:
                if (index != -1) {
                    players[index].active = false;
                    uart_printf(CONSOLE, "Player %d quit.\r\n", sender_tid);
                    // Handle the case if player was in a game
                    resolveGame(index, -1); // Force resolve with no opponent
                }
                break;
        }
        YIELD();  // Yield CPU to other tasks
    }
}

void handlePlay(int index, int choice) {
    players[index].choice = choice;
    uart_printf(CONSOLE, "Player %d played choice %d.\r\n", players[index].tid, choice);

    // Check for a match
    for (int i = 0; i < player_count; ++i) {
        if (i != index && players[i].choice != -1 && players[i].active) {
            resolveGame(index, i);
            break;
        }
    }
}

void resolveGame(int p1_index, int p2_index) {
    if (p2_index == -1) {  // No opponent to match
        REPLY(players[p1_index].tid, "WAIT", 5); // Tell player to wait
        return;
    }

    int result1, result2;
    if (players[p1_index].choice == players[p2_index].choice) {
        result1 = result2 = 0; // Draw
    } else if ((players[p1_index].choice + 1) % 3 == players[p2_index].choice) {
        result1 = -1; // p1 loses
        result2 = 1;  // p2 wins
    } else {
        result1 = 1;  // p1 wins
        result2 = -1; // p2 loses
    }

    // Reply to both players with the result
    REPLY(players[p1_index].tid, (char*)&result1, sizeof(result1));
    REPLY(players[p2_index].tid, (char*)&result2, sizeof(result2));

    // Reset choices after the game
    players[p1_index].choice = players[p2_index].choice = -1;
}
