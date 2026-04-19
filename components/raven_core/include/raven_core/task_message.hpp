#pragma once

#include <cstdint>

// TaskMessage — minimal directed message posted to a task's inbound queue.
//
// kind and id together identify what the message represents.
// data is an optional unowned payload pointer for additional context.
//
// Events (broadcast facts) and task messages (directed commands) are
// intentionally distinct concepts in this runtime.


namespace raven {

struct TaskMessage {
    uint16_t kind;         // message category
    uint16_t id;           // numeric identifier within the category
    void*    data;         // owned payload buffer, may be nullptr
    size_t   payload_size; // size of owned payload buffer in bytes
};

} // namespace raven

namespace raven::net_msg {
    constexpr uint16_t JOYSTICK_INPUT        = 0x1001;
    constexpr uint16_t START_MANUAL_NAV      = 0x1002;
    constexpr uint16_t HALT_MANUAL_NAV       = 0x1003;
    constexpr uint16_t LLM_RESPONSE_TEXT     = 0x2001;
}

namespace raven::msg_kind {
    constexpr uint16_t PILOT_INPUT = 0x0100;
    constexpr uint16_t NAV_CMD     = 0x0200;
    constexpr uint16_t LLM_DATA    = 0x0300;
}

namespace raven {

struct JoystickPayload {
    float    x;
    float    y;
    uint32_t timestamp_ms;
};

struct StartManualPayload {
    uint32_t timestamp_ms;
};

struct HaltManualPayload {
    uint32_t timestamp_ms;
};

} // namespace raven
