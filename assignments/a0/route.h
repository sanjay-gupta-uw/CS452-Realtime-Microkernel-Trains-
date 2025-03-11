#ifndef ROUTE_H
#define ROUTE_H

#include "track_node.h"
#include "track_data.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_QUEUE 50
#define MAX_SWITCHES 22
#define MAX_DEPTH 50

typedef struct {
    track_node* node;
    int distance;
    uint32_t configured_switches; // Bitmask for configured switches (1=configured)
    uint32_t switch_directions;   // Bitmask for switch directions (1=curved)
} BFSState;

typedef struct {
    track_node* node;
    int distance;
    uint32_t configured_switches; // Bitmask for configured switches (1=configured)
    uint32_t switch_directions;   // Bitmask for switch directions (1=curved)
} DFSState;

bool find_path_BFS(track_node track[], track_node* start, track_node* dest, 
              SwitchSetting* switches_set, int* num_switches, int* total_dist);
bool find_path_DFS(track_node track[], track_node* start, track_node* dest,
              SwitchSetting* switches_set, int* num_switches, int* total_dist);
#endif // ROUTE_H