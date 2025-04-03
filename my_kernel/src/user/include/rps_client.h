#ifndef RPS_CLIENT_H
#define RPS_CLIENT_H

enum RPSRequestType
{
    SIGNUP,
    PLAY,
    QUIT_GAME
};
enum GameMove
{
    ROCK,
    PAPER,
    SCISSORS,
    QUIT
};

struct GameRequest
{
    RPSRequestType type;
    GameMove move;
};

// Function to start an RPS game client
void RpsClient(GameMove *moves, int num_moves);

#endif // RPS_CLIENT_H
