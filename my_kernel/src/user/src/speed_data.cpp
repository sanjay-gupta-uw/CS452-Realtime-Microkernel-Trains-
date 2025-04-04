#include "../include/speed_data.h"
#include "../include/uassert.h"

void SpeedData::InitializeTrackA() {
    active_track_ = 'A';
    
    // Train 77 (index 0)                  // Speed | Speed x10 | Stop Dist
    track_a_.entries[0][0] = {223, 272};   // 7: 2.23 mm/tick, 271.55 mm
    track_a_.entries[0][1] = {273, 394};   // 8: 2.73 mm/tick, 394.15 mm
    track_a_.entries[0][2] = {324, 528};   // 9: 3.24 mm/tick, 528.35 mm
    track_a_.entries[0][3] = {385, 685};   // 10: 3.85 mm/tick, 684.87 mm
    track_a_.entries[0][4] = {453, 901};   // 11: 4.53 mm/tick, 901.21 mm
    track_a_.entries[0][5] = {514, 1129};  // 12: 5.14 mm/tick, 1129.11 mm
    track_a_.entries[0][6] = {593, 1460};  // 13: 5.93 mm/tick, 1459.81 mm
    track_a_.entries[0][7] = {617, 1582};  // 14: 6.17 mm/tick, 1581.61 mm

    // Train 54 (index 1)
    track_a_.entries[1][0] = {318, 462};   // 7: 3.18 mm/tick, 462.13 mm
    track_a_.entries[1][1] = {367, 503};   // 8: 3.67 mm/tick, 502.6 mm
    track_a_.entries[1][2] = {412, 562};   // 9: 4.12 mm/tick, 561.73 mm
    track_a_.entries[1][3] = {467, 712};   // 10: 4.67 mm/tick, 612.3 mm
    track_a_.entries[1][4] = {513, 790};   // 11: 5.13 mm/tick, 705.39 mm
    track_a_.entries[1][5] = {572, 866};   // 12: 5.72 mm/tick, 766.35 mm
    track_a_.entries[1][6] = {603, 847};   // 13: 6.03 mm/tick, 847.03 mm
    track_a_.entries[1][7] = {605, 829};   // 14: 6.05 mm/tick, 828.57 mm

    // Train 55 (index 2)
    track_a_.entries[2][0] = {160, 131};   // 7: 1.6 mm/tick, 130.75 mm
    track_a_.entries[2][1] = {200, 174};   // 8: 2.0 mm/tick, 174.09 mm
    track_a_.entries[2][2] = {255, 246};   // 9: 2.55 mm/tick, 245.86 mm
    track_a_.entries[2][3] = {311, 363};   // 10: 3.11 mm/tick, 362.72 mm
    track_a_.entries[2][4] = {371, 494};   // 11: 3.71 mm/tick, 493.66 mm
    track_a_.entries[2][5] = {428, 662};   // 12: 4.28 mm/tick, 661.69 mm
    track_a_.entries[2][6] = {489, 841};   // 13: 4.89 mm/tick, 841.33 mm
    track_a_.entries[2][7] = {523, 954};   // 14: 5.23 mm/tick, 954.34 mm

    // Train 58 (index 3)
    track_a_.entries[3][0] = {173, 108};   // 7: 1.73 mm/tick, 108.25 mm
    track_a_.entries[3][1] = {220, 233};   // 8: 2.2 mm/tick, 233.32 mm
    track_a_.entries[3][2] = {280, 252};   // 9: 2.8 mm/tick, 252.4 mm
    track_a_.entries[3][3] = {339, 450};   // 10: 3.39 mm/tick, 450.39 mm
    track_a_.entries[3][4] = {416, 716};   // 11: 4.16 mm/tick, 715.6 mm
    track_a_.entries[3][5] = {482, 821};   // 12: 4.82 mm/tick, 821.26 mm
    track_a_.entries[3][6] = {550, 1068};  // 13: 5.5 mm/tick, 1067.56 mm
    track_a_.entries[3][7] = {605, 1246};  // 14: 6.05 mm/tick, 1245.87 mm
}

void SpeedData::InitializeTrackB() {
    active_track_ = 'B';
    // using track A for now
}

void SpeedData::SetActiveTrack(char track) {
    active_track_ = track;
}

int SpeedData::GetSpeed(int train_number, int speed_level) const {
    int t_idx = GetTrainIndex(train_number);
    int s_idx = GetSpeedIndex(speed_level);
    uassert(t_idx != -1 && s_idx != -1 && "Invalid train or speed");
    
    return GetActiveTrack().entries[t_idx][s_idx].actual_speed_x100;
}

int SpeedData::GetStoppingDistance(int train_number, int speed_level) const {
    int t_idx = GetTrainIndex(train_number);
    int s_idx = GetSpeedIndex(speed_level);
    uassert(t_idx != -1 && s_idx != -1 && "Invalid train or speed");
    
    return GetActiveTrack().entries[t_idx][s_idx].stopping_distance;
}

void SpeedData::UpdateSpeed(int train_number, int speed_level, int actual_speed_x100) {
    int t_idx = GetTrainIndex(train_number);
    int s_idx = GetSpeedIndex(speed_level);
    uassert(t_idx != -1 && s_idx != -1 && "Invalid train or speed");
    
    CalData& active = const_cast<CalData&>(GetActiveTrack());
    active.entries[t_idx][s_idx].actual_speed_x100 = actual_speed_x100;
}

void SpeedData::UpdateStoppingDistance(int train_number, int speed_level, int distance) {
    int t_idx = GetTrainIndex(train_number);
    int s_idx = GetSpeedIndex(speed_level);
    uassert(t_idx != -1 && s_idx != -1 && "Invalid train or speed");
    
    CalData& active = const_cast<CalData&>(GetActiveTrack());
    active.entries[t_idx][s_idx].stopping_distance = distance;
}


int SpeedData::GetTrainIndex(int train_number) const {
    for (int i = 0; i < CAL_NUM_TRAINS; ++i) {
        if (SUPPORTED_TRAINS[i] == train_number) return i;
    }
    return -1;
}

int SpeedData::GetSpeedIndex(int speed_level) const {
    return (speed_level >= 7 && speed_level <= 14) ? speed_level - 7 : -1;
}

const SpeedData::CalData& SpeedData::GetActiveTrack() const {
    return (active_track_ == 'A') ? track_a_ : track_b_;
}