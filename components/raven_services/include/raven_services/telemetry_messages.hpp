#pragma once

#include <cstdint>

// telemetry_messages.hpp — message IDs, kind constants, and payload structs for
// the telemetry domain.
//
// TELEMETRY_DATA (0x2001):  outbound — packed telemetry snapshot sent to the
//                            remote server via NetworkClientGateway.
// TELEMETRY_ACTIVE (0x2002): inbound  — enables or disables TelemetryService
//                            data collection and transmission.

namespace raven::net_msg {
    constexpr uint16_t TELEMETRY_DATA   = 0x2001;  // outbound telemetry snapshot
    constexpr uint16_t TELEMETRY_ACTIVE = 0x2002;  // inbound activation toggle
} // namespace raven::net_msg

namespace raven::msg_kind {
    constexpr uint16_t TELEMETRY = 0x0400;
} // namespace raven::msg_kind

namespace raven {

// Packed wire payload for a complete telemetry snapshot (outbound).
// All three telemetry categories are serialised into a single flat struct so
// the receiver can parse the full snapshot from one contiguous buffer.
#pragma pack(push, 1)
struct TelemetryPayload {
    // Navigation segment
    float    speed;
    float    direction;
    float    position_x;
    float    position_y;
    uint32_t nav_timestamp;

    // System segment
    int32_t  rssi;
    uint32_t heap_free;
    float    battery_pct;
    uint32_t sys_timestamp;

    // Sensor segment
    float    lidar_distance;
    float    humidity;
    float    temperature;
    float    noise_level;
    uint32_t sensor_timestamp;
};
#pragma pack(pop)

// Inbound payload for TELEMETRY_ACTIVE messages.
// active == true  → start collecting and transmitting telemetry.
// active == false → stop collecting and transmitting telemetry.
struct TelemetryActivePayload {
    bool active;
};

} // namespace raven
