#ifndef TRACK_NODE_H
#define TRACK_NODE_H

typedef enum
{
    NODE_NONE,
    NODE_SENSOR,
    NODE_BRANCH,
    NODE_MERGE,
    NODE_ENTER,
    NODE_EXIT,
} track_node_type;

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1

typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_node;
struct track_edge
{
    track_edge *reverse;
    track_node *src, *dest;
    int dist; /* in millimetres */
};

struct track_node
{
    const char *name;
    track_node_type type;
    int num;             /* sensor or switch number */
    track_node *reverse; /* same location, but opposite direction */
    track_edge edge[2];  // VERIFY IF THIS IS NEEDED -- SINCE POINTER TO REVERSE IS ALREADY THERE
    bool is_node_in_loop;
};

typedef enum
{
    SWITCH_STRAIGHT,
    SWITCH_CURVED
} SwitchDirection;

typedef struct
{
    int switch_num;
    SwitchDirection dir;
} SwitchSetting;

#endif // TRACK_NODE_H