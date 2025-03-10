#include "route.h"
#include "rpi.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_QUEUE 144

// Helper functions for switch number <-> bit position mapping
static int switch_to_bit(int switch_num) {
    if (switch_num >= 1 && switch_num <= 18) {
        return switch_num - 1;
    }
    if (switch_num >= 153 && switch_num <= 156) {
        return (switch_num - 153) + 18;
    }
    return -1;
}

static int bit_to_switch(int bit) {
    if (bit >= 0 && bit <= 17) {
        return bit + 1;
    }
    if (bit >= 18 && bit <= 21) {
        return 153 + (bit - 18);
    }
    return -1;
}

bool find_path_BFS(track_node track[], track_node* start, track_node* dest, 
              SwitchSetting* switches_set, int* num_switches, int* total_dist) {
    BFSState queue[MAX_QUEUE];
    int front = 0, rear = 0;

    // Initialize queue with the start node
    queue[rear++] = (BFSState){start, 0, 0, 0};

    while (front < rear) {
        BFSState current = queue[front++];

        // Check if we've reached the destination
        if (current.node == dest) {
            *total_dist = current.distance;
            *num_switches = 0;

            // Convert bitmask to switch settings
            for (int bit = 0; bit < 22; bit++) {
                if (current.configured_switches & (1 << bit)) {
                    int switch_num = bit_to_switch(bit);
                    if (switch_num != -1) {
                        SwitchDirection dir = (current.switch_directions >> bit) & 1;
                        switches_set[(*num_switches)++] = (SwitchSetting){
                            .switch_num = switch_num,
                            .dir = dir
                        };
                    }
                }
            }
            return true;
        }

        track_node* node = current.node;

        // Handle branch nodes (switches)
        if (node->type == NODE_BRANCH) {
            int switch_num = node->num;
            int bit = switch_to_bit(switch_num);
            
            if (bit == -1) {
                continue; // Skip invalid switches
            }

            if (!(current.configured_switches & (1 << bit))) {
                // Explore both directions since switch isn't configured yet
                for (int dir = 0; dir < 2; dir++) {
                    BFSState new_state = current;
                    
                    // Mark switch as configured
                    new_state.configured_switches |= (1 << bit);
                    
                    // Set direction bit
                    if (dir) {
                        new_state.switch_directions |= (1 << bit); // Curved
                    } else {
                        new_state.switch_directions &= ~(1 << bit); // Straight
                    }
                    
                    track_edge* edge = &node->edge[dir];
                    new_state.node = edge->dest;
                    new_state.distance += edge->dist;
                    
                    if (rear < MAX_QUEUE) {
                        queue[rear++] = new_state;
                    }
                }
            } else {
                // Use existing configuration
                int dir = (current.switch_directions >> bit) & 1;
                track_edge* edge = &node->edge[dir];
                BFSState new_state = current;
                new_state.node = edge->dest;
                new_state.distance += edge->dist;
                
                if (rear < MAX_QUEUE) {
                    queue[rear++] = new_state;
                }
            }
        } else {
            // Handle all other node types (follow straight ahead)
            track_edge* edge = &node->edge[DIR_AHEAD];
            BFSState new_state = current;
            new_state.node = edge->dest;
            new_state.distance += edge->dist;
            
            if (rear < MAX_QUEUE) {
                queue[rear++] = new_state;
            }
        }
    }
    return false;
}