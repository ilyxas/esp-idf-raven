#pragma once

#include "raven_core/task_message.hpp"

#include <cstddef>
#include <cstdint>

namespace raven {

// EncodedView — lightweight non-owning view of an encoded outbound payload.
// The referenced buffer is valid only for the duration of the encode() call.
struct EncodedView {
    const void* payload      = nullptr;
    size_t      payload_size = 0;
};

// IEncoder — outbound wire encoder interface.
//
// Mirrors IDecoder on the inbound side.
// Implementations inspect a TaskMessage and populate an EncodedView with the
// raw bytes to be written into the NetworkHeader payload field.
class IEncoder {
public:
    virtual ~IEncoder() = default;

    // Populate `out` with the wire payload derived from `msg`.
    // Returns true on success, false if the message cannot be encoded.
    virtual bool encode(const TaskMessage& msg, EncodedView& out) const = 0;
};

// RawEncoder — passes the TaskMessage data buffer through as-is.
//
// Suitable when the TaskMessage already carries a serialised binary payload,
// e.g. a struct copied into the queue message.
class RawEncoder final : public IEncoder {
public:
    bool encode(const TaskMessage& msg, EncodedView& out) const override
    {
        out.payload      = msg.data;
        out.payload_size = msg.payload_size;
        return true;
    }
};

} // namespace raven
