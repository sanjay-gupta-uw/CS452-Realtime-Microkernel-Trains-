#include "rps_server.h"
#include "rps_client.h"
#include "name_server.h"
#include "../kern/syscall.h"
#include "../rpi.h"
#include <cstring>

#define MAX_PLAYERS 10

struct Player {
    int tid;
    int choice = -1;
    bool active = false;
    bool inGame = false;
};

Player players[MAX_PLAYERS];
int player_count = 0;

void matchPlayers();
void handlePlay(int index, int choice);
void resolveGame(int p1_index, int p2_index);

void RpsServer() {
    uart_printf(CONSOLE, "RPS Server running...\r\n");
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
                    players[player_count] = {sender_tid, -1, true, false};
                    player_count++;
                    uart_printf(CONSOLE, "Player %d signed up.\r\n", sender_tid);
                    matchPlayers(); // Check if two players can be paired
                } else {
                    REPLY(sender_tid, "FULL", 5);  // Server full
                }
                break;
            case PLAY:
                if (index != -1 && players[index].active && players[index].inGame) {
                    handlePlay(index, req.move);
                }
                break;
            case QUIT_GAME:
                if (index != -1) {
                    int p2_index = -1;
                    for (int j = 0; j < player_count; ++j) {
                        if (j != index && players[j].inGame && players[index].inGame && players[j].tid != players[index].tid) {
                            p2_index = j;
                            break;
                        }
                    }
                    players[index].active = false;
                    players[index].inGame = false;
                    uart_printf(CONSOLE, "Player %d quit.\r\n", sender_tid);
                    if (p2_index != -1) { // Notify the other player if in game
                        players[p2_index].inGame = false;
                        REPLY(players[p2_index].tid, "OTHER_QUIT", 11);
                        uart_printf(CONSOLE, "Notifying Player %d of the other's quit.\r\n", players[p2_index].tid);
                    }
                    resolveGame(index, -1); // Resolve game if mid-match
                    matchPlayers(); // Attempt to match remaining players
                }
                break;
        }
        YIELD();  // Yield CPU to other tasks
    }
}

const char* getMoveName(int move) {
    switch (move) {
        case ROCK: return "Rock";
        case PAPER: return "Paper";
        case SCISSORS: return "Scissors";
        default: return "Unknown";
    }
}

const char* getResultDescription(int result) {
    switch (result) {
        case 0: return "Draw";
        case 1: return "Wins";
        case -1: return "Loses";
        default: return "Error";
    }
}

void matchPlayers() {
    int first = -1;
    for (int i = 0; i < player_count; ++i) {
        if (players[i].active && !players[i].inGame && first == -1) {
            first = i;
        } else if (players[i].active && !players[i].inGame && first != -1) {
            // Pair the players
            players[first].inGame = players[i].inGame = true;
            REPLY(players[first].tid, "PAIRED", 7);
            REPLY(players[i].tid, "PAIRED", 7);
            uart_printf(CONSOLE, "Players %d and %d are paired for a game.\r\n", players[first].tid, players[i].tid);
            return;
        }
    }
}

void handlePlay(int index, int choice) {
    players[index].choice = choice;
    uart_printf(CONSOLE, "Player %d played %s.\r\n", players[index].tid, getMoveName(choice));

    for (int i = 0; i < player_count; ++i) {
        if (i != index && players[i].choice != -1 && players[i].active && players[i].inGame) {
            resolveGame(index, i);
            break;
        }
    }
}

void resolveGame(int p1_index, int p2_index) {
    if (p2_index == -1) {
        REPLY(players[p1_index].tid, "WAIT", 5);
        uart_printf(CONSOLE, "Game unresolved for Player %d: Waiting for an opponent.\r\n", players[p1_index].tid);
        return;
    }

    const char* result1Desc = getResultDescription(players[p1_index].choice == players[p2_index].choice ? 0 :
                                                   (players[p1_index].choice + 1) % 3 == players[p2_index].choice ? -1 : 1);
    const char* result2Desc = getResultDescription(players[p1_index].choice == players[p2_index].choice ? 0 :
                                                   (players[p1_index].choice + 1) % 3 == players[p2_index].choice ? 1 : -1);

    uart_printf(CONSOLE, "Game result between Player %d and Player %d: %s, %s\r\n",
                players[p1_index].tid, players[p2_index].tid, result1Desc, result2Desc);

    REPLY(players[p1_index].tid, result1Desc, strlen(result1Desc) + 1);
    REPLY(players[p2_index].tid, result2Desc, strlen(result2Desc) + 1);

    players[p1_index].choice = players[p2_index].choice = -1;
    players[p1_index].inGame = players[p2_index].inGame = false;
}
