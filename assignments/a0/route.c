#include "route.h"
#include "rpi.h"
#include <string.h>
#include <stdbool.h>

bool find_path(track_node track[], track_node* start, track_node* dest, 
              SwitchSetting* switches_set, int* num_switches, int* total_dist) {
    static BFSState queue[MAX_QUEUE];
    int front = 0, rear = 0;

    // uart_printf(CONSOLE, "\r\nStarting pathfinding from %s to %s\r\n", 
    //           start->name, dest->name);

    // Initialize queue with the start node
    BFSState initial = {start, 0, {{0}}, 0};
    queue[rear++] = initial;
    // uart_printf(CONSOLE, "  Enqueued initial state: %s (dist %d, switches_set %d)\r\n", 
    //           start->name, 0, 0);

    while (front < rear) {
        BFSState current = queue[front++];
        // uart_printf(CONSOLE, "\r\nProcessing state: %s (dist %d, switches_set %d)\r\n",
        //           current.node->name, current.distance, current.num_switches);

        // Check if we've reached the destination
        if (current.node == dest) {
            // uart_printf(CONSOLE, "  FOUND DESTINATION! Total distance: %d\r\n", current.distance);
            // uart_printf(CONSOLE, "  switches_set required (%d):\r\n", current.num_switches);
            for (int i = 0; i < current.num_switches; i++) {
                // uart_printf(CONSOLE, "    Switch %d: %s\r\n", 
                //           current.switches_set[i].switch_num,
                //           current.switches_set[i].dir == SWITCH_STRAIGHT ? "straight" : "curved");
            }
            
            *total_dist = current.distance;
            *num_switches = current.num_switches;
            memcpy(switches_set, current.switches_set, current.num_switches * sizeof(SwitchSetting));
            return true;
        }

        track_node* current_node = current.node;

        // Handle branch nodes (switches)
        if (current_node->type == NODE_BRANCH) {
            // uart_printf(CONSOLE, "  Processing BRANCH node %s (%d)\r\n", 
            //           current_node->name, current_node->num);
            
            int switch_num = current_node->num;
            int switch_index = -1;

            // Check if this switch was already set in the current path
            for (int s = 0; s < current.num_switches; s++) {
                if (current.switches_set[s].switch_num == switch_num) {
                    switch_index = s;
                    break;
                }
            }

            if (switch_index != -1) {
                SwitchDirection dir = current.switches_set[switch_index].dir;
                // uart_printf(CONSOLE, "  Using existing switch setting: %d -> %s\r\n",
                //           switch_num, dir == SWITCH_STRAIGHT ? "straight" : "curved");
                
                int edge_index = (dir == SWITCH_STRAIGHT) ? 0 : 1;
                track_edge* edge = &current_node->edge[edge_index];
                track_node* next = edge->dest;

                BFSState new_state = current;
                new_state.node = next;
                new_state.distance += edge->dist;

                // uart_printf(CONSOLE, "  Adding path to %s (new dist %d)\r\n",
                //           next->name, new_state.distance);

                if (rear < MAX_QUEUE) {
                    queue[rear++] = new_state;
                } else {
                    // uart_printf(CONSOLE, "  QUEUE FULL! Can't add %s\r\n", next->name);
                }
            } else {
                // uart_printf(CONSOLE, "  New switch %d - exploring both directions\r\n", switch_num);
                
                for (int edge_idx = 0; edge_idx < 2; edge_idx++) {
                    BFSState new_state = current;
                    new_state.switches_set[new_state.num_switches++] = (SwitchSetting){
                        .switch_num = switch_num,
                        .dir = (edge_idx == 0) ? SWITCH_STRAIGHT : SWITCH_CURVED
                    };

                    track_edge* edge = &current_node->edge[edge_idx];
                    track_node* next = edge->dest;
                    new_state.node = next;
                    new_state.distance += edge->dist;

                    // uart_printf(CONSOLE, "  Exploring %s direction: %s -> %s (dist %d)\r\n",
                    //           edge_idx == 0 ? "straight" : "curved",
                    //           current_node->name, next->name, new_state.distance);

                    if (rear < MAX_QUEUE) {
                        queue[rear++] = new_state;
                    } else {
                        // uart_printf(CONSOLE, "  QUEUE FULL! Can't add %s\r\n", next->name);
                    }
                }
            }
        } else {
            // Handle all other node types (single edge)
            track_edge* edge = &current_node->edge[DIR_AHEAD];
            track_node* next = edge->dest;

            // uart_printf(CONSOLE, "  Following edge from %s to %s (dist %d)\r\n",
            //           current_node->name, next->name, current.distance + edge->dist);

            BFSState new_state = current;
            new_state.node = next;
            new_state.distance += edge->dist;

            if (rear < MAX_QUEUE) {
                queue[rear++] = new_state;
            } else {
                // uart_printf(CONSOLE, "  QUEUE FULL! Can't add %s\r\n", next->name);
            }
        }

        // uart_printf(CONSOLE, "  Queue status: front=%d, rear=%d\r\n", front, rear);
    }

    // uart_printf(CONSOLE, "\r\nPATHFINDING FAILED! No path found\r\n");
    return false;
}

