#pragma once

#include "raven_services/base_service.hpp"
#include "raven_services/telemetry_messages.hpp"
#include "raven_state/telemetry_state.hpp"
#include "raven_gateways/network_client_gateway.hpp"

#include "freertos/FreeRTOS.h"

namespace raven {

// TelemetryService — periodic telemetry collector and transmitter.
//
// Responsibilities:
//   • Every 100 ms (on_tick) collect Navigation, System, and Sensor data,
//     write them into TelemetryState, then assemble a TelemetryPayload and
//     post it to NetworkClientGateway for transmission (msgId = 0x2001).
//   • Reacts to TELEMETRY_ACTIVE (msgId = 0x2002) messages that toggle
//     data collection and transmission on or off.
//
// Real sensor or hardware sources are absent in this reference build;
// placeholder values (zeros, esp_get_free_heap_size, etc.) are used instead.
class TelemetryService : public BaseService {
public:
    explicit TelemetryService(TelemetryState& state, NetworkClientGateway& gateway);

protected:
    // Enables the 100 ms periodic tick.
    void on_start() override;

    // Handles TELEMETRY_ACTIVE toggle messages.
    void handle_message(const TaskMessage& msg) override;

    // Called every kTickInterval when active_: collects data, updates
    // TelemetryState, and transmits a snapshot via NetworkClientGateway.
    void on_tick() override;

private:
    void collect_navigation();
    void collect_system();
    void collect_sensor();
    void transmit();

    TelemetryState&       state_;
    NetworkClientGateway& gateway_;
    bool                  active_{true};

    // Payload buffer reused across ticks to avoid repeated heap allocation.
    TelemetryPayload payload_{};

    static constexpr TickType_t kTickInterval = pdMS_TO_TICKS(100);
};

} // namespace raven
