

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <deque>

class PlayerShip {
public:
    struct Snapshot {
        std::vector<float> position;
        std::vector<float> rotation;
        std::vector<float> velocity;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::string player_id;
    std::string ship_id;
    std::vector<float> position; // x, y, z
    std::vector<float> rotation; // pitch, yaw, roll
    std::vector<float> velocity; // vx, vy, vz
    bool is_remote;

    // Snapshot interpolation data
    std::vector<float> previous_position;
    std::vector<float> previous_rotation;
    std::chrono::steady_clock::time_point last_update_time;
    std::chrono::steady_clock::time_point interpolation_start_time;
    std::deque<Snapshot> snapshots; // Circular buffer of recent snapshots

    PlayerShip();
    PlayerShip(const std::string& p_id, const std::string& s_id, bool remote = false);

    // Update ship state with new position data
    void UpdatePosition(const std::vector<float>& new_position,
                       const std::vector<float>& new_rotation,
                       const std::vector<float>& new_velocity);

    // Get interpolated position for smooth rendering
    std::vector<float> GetInterpolatedPosition(float interpolation_delay_ms = 100.0f) const;
    std::vector<float> GetInterpolatedRotation(float interpolation_delay_ms = 100.0f) const;

    // Check if ship data is stale (for cleanup)
    bool IsStale(std::chrono::milliseconds max_age = std::chrono::milliseconds(5000)) const;

    // Legacy methods for compatibility
    void Update();
    void SetPosition(float x, float y, float z);
    void SetRotation(float pitch, float yaw, float roll);
    void SetVelocity(float vx, float vy, float vz);
    void SectorChanged(const std::string& newSector);
};