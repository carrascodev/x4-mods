#include "../public/player_ship.h"
#include <chrono>
#include <algorithm>

PlayerShip::PlayerShip()
    : player_id("")
    , ship_id("")
    , position{0.0f, 0.0f, 0.0f}
    , rotation{0.0f, 0.0f, 0.0f}
    , velocity{0.0f, 0.0f, 0.0f}
    , is_remote(false)
    , last_update_time(std::chrono::steady_clock::now())
    , interpolation_start_time(std::chrono::steady_clock::now())
{
}

PlayerShip::PlayerShip(const std::string& p_id, const std::string& s_id, bool remote)
    : player_id(p_id)
    , ship_id(s_id)
    , position{0.0f, 0.0f, 0.0f}
    , rotation{0.0f, 0.0f, 0.0f}
    , velocity{0.0f, 0.0f, 0.0f}
    , is_remote(remote)
    , last_update_time(std::chrono::steady_clock::now())
    , interpolation_start_time(std::chrono::steady_clock::now())
{
}

void PlayerShip::UpdatePosition(const std::vector<float>& new_position,
                               const std::vector<float>& new_rotation,
                               const std::vector<float>& new_velocity)
{
    // Store previous state for interpolation
    previous_position = position;
    previous_rotation = rotation;

    // Update current state
    position = new_position;
    rotation = new_rotation;
    velocity = new_velocity;

    last_update_time = std::chrono::steady_clock::now();

    // Reset interpolation if this is a new snapshot
    if (snapshots.size() < 2) {
        interpolation_start_time = last_update_time;
    }

    // Add to snapshot buffer for interpolation
    Snapshot snapshot;
    snapshot.position = position;
    snapshot.rotation = rotation;
    snapshot.velocity = velocity;
    snapshot.timestamp = last_update_time;

    snapshots.push_back(snapshot);

    // Keep only recent snapshots (last 1 second worth)
    const auto cutoff_time = last_update_time - std::chrono::milliseconds(1000);
    snapshots.erase(
        std::remove_if(snapshots.begin(), snapshots.end(),
            [cutoff_time](const Snapshot& s) { return s.timestamp < cutoff_time; }),
        snapshots.end()
    );
}

std::vector<float> PlayerShip::GetInterpolatedPosition(float interpolation_delay_ms) const
{
    if (!is_remote || snapshots.size() < 2) {
        return position;
    }

    // Find snapshots to interpolate between
    const auto render_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(static_cast<int>(interpolation_delay_ms));

    // Find the two snapshots to interpolate between
    size_t older_index = 0;
    size_t newer_index = 1;

    for (size_t i = 0; i < snapshots.size() - 1; ++i) {
        if (snapshots[i].timestamp <= render_time && snapshots[i + 1].timestamp >= render_time) {
            older_index = i;
            newer_index = i + 1;
            break;
        }
    }

    if (older_index >= snapshots.size() - 1) {
        return snapshots.back().position;
    }

    const auto& older = snapshots[older_index];
    const auto& newer = snapshots[newer_index];

    // Calculate interpolation factor
    const auto time_diff = newer.timestamp - older.timestamp;
    const auto elapsed = render_time - older.timestamp;
    const float t = time_diff.count() > 0 ?
        static_cast<float>(elapsed.count()) / static_cast<float>(time_diff.count()) : 0.0f;

    // Clamp t to [0, 1]
    const float clamped_t = std::max(0.0f, std::min(1.0f, t));

    // Interpolate position
    std::vector<float> interpolated_pos(3);
    for (size_t i = 0; i < 3; ++i) {
        interpolated_pos[i] = older.position[i] + (newer.position[i] - older.position[i]) * clamped_t;
    }

    return interpolated_pos;
}

std::vector<float> PlayerShip::GetInterpolatedRotation(float interpolation_delay_ms) const
{
    if (!is_remote || snapshots.size() < 2) {
        return rotation;
    }

    const auto render_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(static_cast<int>(interpolation_delay_ms));

    size_t older_index = 0;
    size_t newer_index = 1;

    for (size_t i = 0; i < snapshots.size() - 1; ++i) {
        if (snapshots[i].timestamp <= render_time && snapshots[i + 1].timestamp >= render_time) {
            older_index = i;
            newer_index = i + 1;
            break;
        }
    }

    if (older_index >= snapshots.size() - 1) {
        return snapshots.back().rotation;
    }

    const auto& older = snapshots[older_index];
    const auto& newer = snapshots[newer_index];

    const auto time_diff = newer.timestamp - older.timestamp;
    const auto elapsed = render_time - older.timestamp;
    const float t = time_diff.count() > 0 ?
        static_cast<float>(elapsed.count()) / static_cast<float>(time_diff.count()) : 0.0f;

    const float clamped_t = std::max(0.0f, std::min(1.0f, t));

    // Interpolate rotation (simple linear interpolation for now)
    std::vector<float> interpolated_rot(3);
    for (size_t i = 0; i < 3; ++i) {
        interpolated_rot[i] = older.rotation[i] + (newer.rotation[i] - older.rotation[i]) * clamped_t;
    }

    return interpolated_rot;
}

bool PlayerShip::IsStale(std::chrono::milliseconds max_age) const
{
    const auto now = std::chrono::steady_clock::now();
    return (now - last_update_time) > max_age;
}