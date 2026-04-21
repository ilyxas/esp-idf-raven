#include "raven_gateways/network_client_gateway.hpp"

#include "esp_log.h"

#include <cstdint>

static const char* TAG = "NetworkClientGateway";

namespace raven {

NetworkClientGateway::NetworkClientGateway(const Config& cfg)
    : BaseTask({ cfg.name, cfg.stack_size, cfg.priority, cfg.queue_length })
    , link_(cfg.host, cfg.port)
{
}

bool NetworkClientGateway::register_encoder(uint16_t kind, uint16_t id, const IEncoder* encoder)
{
    return encoders_.register_encoder(kind, id, encoder);
}

void NetworkClientGateway::handle_message(const TaskMessage& msg)
{
    const IEncoder* encoder = encoders_.find(msg.kind, msg.id);
    if (encoder == nullptr) {
        ESP_LOGW(TAG, "No encoder registered for kind=%u id=%u — dropping message", msg.kind, msg.id);
        return;
    }

    EncodedView view {};
    if (!encoder->encode(msg, view)) {
        ESP_LOGW(TAG, "Encoder failed for kind=%u id=%u — dropping message", msg.kind, msg.id);
        return;
    }

    if (!ensure_connected()) {
        ESP_LOGW(TAG, "Could not connect to remote server — dropping message");
        return;
    }

    if (!write_frame(msg.id, view.payload, view.payload_size)) {
        ESP_LOGW(TAG, "Write failed — closing link");
        link_.close();
    }
}

bool NetworkClientGateway::ensure_connected()
{
    if (link_.isOpen()) {
        return true;
    }

    const bool ok = link_.open();
    if (!ok) {
        ESP_LOGW(TAG, "Failed to open TCP link");
    }
    return ok;
}

bool NetworkClientGateway::write_frame(uint16_t msg_id, const void* payload, size_t payload_size)
{
    NetworkHeader header {};
    header.msg_id       = msg_id;
    header.payload_size = static_cast<uint32_t>(payload_size);

    const ssize_t hw = link_.write(&header, sizeof(header));
    if (hw != static_cast<ssize_t>(sizeof(header))) {
        return false;
    }

    if (payload_size > 0 && payload != nullptr) {
        const ssize_t pw = link_.write(payload, payload_size);
        if (pw != static_cast<ssize_t>(payload_size)) {
            return false;
        }
    }

    return true;
}

} // namespace raven
