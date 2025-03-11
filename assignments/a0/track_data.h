/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */
#ifndef TRACK_DATA_H
#define TRACK_DATA_H

#include "track_node.h"

// The track initialization functions expect an array of this size.
#define TRACK_MAX 144

void init_tracka(track_node *track);
void init_trackb(track_node *track);
int get_node_num_by_name_b(const char* name);
track_node* find_node_by_sensor_index(track_node track[], int idx);
track_node* find_node_by_name(track_node track[], const char* name);


#endif // TRACK_DATA_H