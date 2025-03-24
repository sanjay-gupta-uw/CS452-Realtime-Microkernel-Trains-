/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */
#ifndef TRACK_DATA_H
#define TRACK_DATA_H

#include "track_node.h"
#include "../marklin/switch.h"
#include "../../containers/stack.h"
#include "../../containers/queue.h"

// The track initialization functions expect an array of this size.
#define TRACK_MAX 144
#define REVERSE_COST 300

class Track
{
private:
    int CONTROLLER_TID;
    track_node track[TRACK_MAX];
    char track_id;
    void init_tracka();
    void init_trackb();
    void initialize_loop(); // sets noid

public:
    Track();
    ~Track();

    void getLoop(Queue<PathNode, NUM_SWITCHES> *switch_config, int *distance);
    void init(char track_id); // initialized either track a or b
    track_node *get_node_by_name(const char *name);
    void find_path(const char *start, const char *dest, Stack<PathNode, TRACK_MAX> *path, bool check_start_dest = true);
    // int get_node_num_by_name_b(const char *name);
};

#endif // TRACK_DATA_H