#include "raven_services/telemetry_service.hpp"
#include "raven_services/telemetry_messages.hpp"
#include "raven_core/task_message.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include <inttypes.h>
#include <cstring>

namespace raven {

static const char* TAG = "TelemetryService";

TelemetryService::TelemetryService(TelemetryState& state, NetworkClientGateway& gateway)
    : BaseService({ "telemetry_svc", 4096, 5, 8 })
    , state_(state)
    , gateway_(gateway)
{
}

void TelemetryService::on_start()
{
    set_tick_interval(kTickInterval);
    ESP_LOGI(TAG, "started — collecting every 100 ms (active=%s)",
             active_ ? "true" : "false");
}

void TelemetryService::handle_message(const TaskMessage& msg)
{
    if (msg.id != net_msg::TELEMETRY_ACTIVE) {
        return;
    }

    if (msg.data == nullptr || msg.payload_size < sizeof(TelemetryActivePayload)) {
        ESP_LOGW(TAG, "TELEMETRY_ACTIVE: invalid payload (data=%p size=%zu)",
                 msg.data, msg.payload_size);
        return;
    }

    const auto* p = static_cast<const TelemetryActivePayload*>(msg.data);
    active_ = p->active;

    ESP_LOGI(TAG, "TELEMETRY_ACTIVE → %s", active_ ? "enabled" : "disabled");
}

void TelemetryService::on_tick()
{
    if (!active_) {
        return;
    }

    collect_navigation();
    collect_system();
    collect_sensor();
    transmit();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void TelemetryService::collect_navigation()
{
    // Placeholder: real data would come from NavigationState or a sensor driver.
    TelemetryState::NavigationTelemetry nav;
    nav.speed      = 0.0f;
    nav.direction  = 0.0f;
    nav.position_x = 0.0f;
    nav.position_y = 0.0f;
    nav.timestamp  = static_cast<uint32_t>(xTaskGetTickCount());

    state_.set_navigation(nav);

    payload_.speed      = nav.speed;
    payload_.direction  = nav.direction;
    payload_.position_x = nav.position_x;
    payload_.position_y = nav.position_y;
    payload_.nav_timestamp = nav.timestamp;
}

void TelemetryService::collect_system()
{
    TelemetryState::SystemTelemetry sys;
    sys.rssi        = 0; // placeholder: no Wi-Fi RSSI API called here
    sys.heap_free   = static_cast<uint32_t>(esp_get_free_heap_size());
    sys.battery_pct = 100.0f; // placeholder
    sys.timestamp   = static_cast<uint32_t>(xTaskGetTickCount());

    state_.set_system(sys);

    payload_.rssi          = sys.rssi;
    payload_.heap_free     = sys.heap_free;
    payload_.battery_pct   = sys.battery_pct;
    payload_.sys_timestamp = sys.timestamp;
}

void TelemetryService::collect_sensor()
{
    TelemetryState::SensorTelemetry sensor;
    sensor.lidar_distance = 0.0f;   // placeholder
    sensor.humidity       = 0.0f;   // placeholder
    sensor.temperature    = 0.0f;   // placeholder
    sensor.noise_level    = 0.0f;   // placeholder
    sensor.timestamp      = static_cast<uint32_t>(xTaskGetTickCount());

    state_.set_sensor(sensor);

    payload_.lidar_distance  = sensor.lidar_distance;
    payload_.humidity        = sensor.humidity;
    payload_.temperature     = sensor.temperature;
    payload_.noise_level     = sensor.noise_level;
    payload_.sensor_timestamp = sensor.timestamp;
}

void TelemetryService::transmit()
{
    TaskMessage msg{};
    msg.kind         = msg_kind::TELEMETRY;
    msg.id           = net_msg::TELEMETRY_DATA;
    msg.data         = &payload_;
    msg.payload_size = sizeof(TelemetryPayload);

    if (!gateway_.post_message(msg)) {
        ESP_LOGW(TAG, "transmit: gateway queue full — dropping snapshot");
    }
}

} // namespace raven
