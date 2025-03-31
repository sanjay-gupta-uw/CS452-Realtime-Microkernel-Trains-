#ifndef TRACK_NODE_H
#define TRACK_NODE_H

#include "../marklin/switch.h"
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

typedef struct
{
    int segment_length;
    track_node *sensor_node;
} SegmentReply;

struct PathNode
{
    track_node *node;
    Switch_NS::SWITCH_STATE switch_state;
};

#endif // TRACK_NODE_H