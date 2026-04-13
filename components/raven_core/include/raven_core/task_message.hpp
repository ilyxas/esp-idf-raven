#pragma once

#include <cstdint>

namespace raven {

// TaskMessage — minimal directed message posted to a task's inbound queue.
//
// kind and id together identify what the message represents.
// data is an optional unowned payload pointer for additional context.
//
// Events (broadcast facts) and task messages (directed commands) are
// intentionally distinct concepts in this runtime.
struct TaskMessage {
    uint16_t kind;  // message category
    uint16_t id;    // numeric identifier within the category
    void*    data;  // optional unowned payload pointer (may be nullptr)
};

} // namespace raven
