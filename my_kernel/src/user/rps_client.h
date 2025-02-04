#ifndef RPS_CLIENT_H
#define RPS_CLIENT_H

enum RequestType { SIGNUP, PLAY, QUIT_GAME };
enum GameMove { ROCK, PAPER, SCISSORS, QUIT };

struct GameRequest {
    RequestType type;  
    GameMove move;  
};


// Function to start an RPS game client
void RpsClient(GameMove *moves, int num_moves);

#endif // RPS_CLIENT_H
