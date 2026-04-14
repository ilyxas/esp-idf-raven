#pragma once

#include <atomic>

namespace raven {

// NavigationState — shared position truth updated by NavigationService.
//
// Services write into this state; Activities and other consumers read from it.
// Atomics provide thread-safe access without requiring a mutex for simple
// scalar reads and writes.
struct NavigationState {
    std::atomic<float> x{0.0f};
    std::atomic<float> y{0.0f};
    std::atomic<float> z{0.0f};
};

} // namespace raven
