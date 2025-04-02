#ifndef SPEED_DATA_H
#define SPEED_DATA_H

class SpeedData {
public:
    // Initialize calibration data for Track A
    void InitializeTrackA();

    // Initialize calibration data for Track B
    void InitializeTrackB();

    // Set active track (A or B)
    void SetActiveTrack(char track);

    // Update speed data for active track
    void UpdateSpeed(int train_number, int speed_level, int actual_speed_x100);

    // Update stopping distance for active track
    void UpdateStoppingDistance(int train_number, int speed_level, int distance);

    // Get calibrated speed (x10) for current track
    int GetSpeed(int train_number, int speed_level) const;

    // Get stopping distance for current track
    int GetStoppingDistance(int train_number, int speed_level) const;

private:
    static constexpr int CAL_NUM_TRAINS = 4;
    static constexpr int SPEED_LEVELS = 8; // 7-14 inclusive
    static constexpr int SUPPORTED_TRAINS[4] = {77, 54, 55, 58};

    struct CalibrationEntry {
        int actual_speed_x100;   // Speed multiplied by 100 (e.g., 150 = 1.50 mm/tick)
        int stopping_distance;  // Stopping distance in millimeters
    };

    struct CalData {
        CalibrationEntry entries[CAL_NUM_TRAINS][SPEED_LEVELS];
    };

    CalData track_a_;
    CalData track_b_;
    char active_track_ = 'A';

    // Helpers
    int GetTrainIndex(int train_number) const;
    int GetSpeedIndex(int speed_level) const;
    const CalData& GetActiveTrack() const;
};

#endif // SPEED_DATA_H