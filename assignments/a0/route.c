#include "route.h"
#include "rpi.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_QUEUE 144
#define MAX_DEPTH 100

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

bool find_path_DFS(track_node track[], track_node* start, track_node* dest,
    SwitchSetting* switches_set, int* num_switches, int* total_dist) {
DFSState stack[MAX_DEPTH];
int top = 0;

// Initialize stack with start node
stack[top++] = (DFSState){start, 0, 0, 0};

while (top > 0) {
DFSState current = stack[--top];  // Pop from top

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

if (top >= MAX_DEPTH - 2) continue;

track_node* node = current.node;

if (node->type == NODE_BRANCH) {
  int switch_num = node->num;
  int bit = switch_to_bit(switch_num);
  
  if (bit == -1) continue;

  if (!(current.configured_switches & (1 << bit))) {
      // Try both directions (prioritize straight first)
      for (int dir = 1; dir >= 0; dir--) {  // Reverse order for stack
          DFSState new_state = current;
          new_state.configured_switches |= (1 << bit);
          if (dir) {
              new_state.switch_directions |= (1 << bit);
          } else {
              new_state.switch_directions &= ~(1 << bit);
          }
          
          track_edge* edge = &node->edge[dir];
          new_state.node = edge->dest;
          new_state.distance += edge->dist;
          
          stack[top++] = new_state;  // Push to stack
      }
  } else {
      // Follow existing configuration
      int dir = (current.switch_directions >> bit) & 1;
      track_edge* edge = &node->edge[dir];
      DFSState new_state = current;
      new_state.node = edge->dest;
      new_state.distance += edge->dist;
      stack[top++] = new_state;
  }
} else {
  // Handle other nodes
  track_edge* edge = &node->edge[DIR_AHEAD];
  DFSState new_state = current;
  new_state.node = edge->dest;
  new_state.distance += edge->dist;
  stack[top++] = new_state;
}
}
return false;
}