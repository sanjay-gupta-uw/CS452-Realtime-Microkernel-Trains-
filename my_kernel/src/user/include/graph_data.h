#ifndef GRAPH_DATA_H
#define GRAPH_DATA_H

#include <cstddef> 

struct SwitchPos {
    int num;
    int line;
    int col;
};

struct TrackConfig {
    const char** diagram;
    size_t diagram_lines;
    const SwitchPos* switches;
    size_t switches_count;
};

extern const char* track_b[];
extern const SwitchPos track_b_switches[];
extern const std::size_t track_b_count;
extern const std::size_t track_b_switches_count;
extern const TrackConfig TRACK_B;

extern const char* track_a[];
extern const SwitchPos track_a_switches[];
extern const std::size_t track_a_count;
extern const std::size_t track_a_switches_count;
extern const TrackConfig TRACK_A;

#endif