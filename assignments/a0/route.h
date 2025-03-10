#ifndef ROUTE_H
#define ROUTE_H

#include "track_node.h"
#include "track_data.h"
#include <stdbool.h>

#define MAX_QUEUE 50
#define MAX_SWITCHES 22

typedef struct {
    track_node* node;
    int distance;
    SwitchSetting switches_set[MAX_SWITCHES];
    int num_switches;
} BFSState;

bool find_path(track_node track[], track_node* start, track_node* dest, 
              SwitchSetting* switches_set, int* num_switches, int* total_dist);
#endif // ROUTE_H