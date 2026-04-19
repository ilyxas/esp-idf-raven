

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "raven_platform/TcpClientLink.hpp"
#include "raven_platform/TcpServer.hpp"

#include "raven_gateways/network_gateway.hpp"
#include "raven_gateways/decoder.hpp"
#include "raven_events/event_bus.hpp"
#include "raven_state/navigation_state.hpp"
#include "raven_services/navigation_service.hpp"
#include "raven_services/navigation_messages.hpp"
#include "raven_activities/navigation_activity.hpp"


#include <cstdint>


namespace {
constexpr const char* TAG = "main";

constexpr const char* WIFI_SSID = "iDevice";
constexpr const char* WIFI_PASS = "12345679";

constexpr const char* SERVER_IP = "172.20.10.1";
constexpr uint16_t SERVER_PORT = 8080;

EventGroupHandle_t s_wifi_event_group = nullptr;
constexpr int WIFI_CONNECTED_BIT = BIT0;
constexpr int WIFI_FAIL_BIT = BIT1;

int s_retry_num = 0;
constexpr int MAXIMUM_RETRY = 10;

void wifi_event_handler(void*,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            ++s_retry_num;
            esp_wifi_connect();
            ESP_LOGI(TAG, "retry to connect to AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool wifi_init_sta() {
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == nullptr) {
        ESP_LOGE(TAG, "failed to create wifi event group");
        return false;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy(reinterpret_cast<char*>(wifi_config.sta.password), WIFI_PASS, sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    wifi_config.sta.disable_wpa3_compatible_mode = 0;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished");

    const EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to SSID:%s", WIFI_SSID);
        return true;
    }

    ESP_LOGE(TAG, "failed to connect to SSID:%s", WIFI_SSID);
    return false;
}

} // namespace

// extern "C" void app_main(void) {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     if (!wifi_init_sta()) {
//         ESP_LOGE(TAG, "wifi init failed");
//         return;
//     }

//     TcpClientLink link(SERVER_IP, SERVER_PORT);

//     if (!link.open()) {
//         ESP_LOGE(TAG, "tcp connect failed");
//         return;
//     }

//     ESP_LOGI(TAG, "tcp connected");

//     const char* msg = "hello from esp32\n";
//     const ssize_t written = link.write(msg, strlen(msg));
//     ESP_LOGI(TAG, "sent bytes: %d", static_cast<int>(written));

//     char buffer[128] = {};
//     const ssize_t n = link.read(buffer, sizeof(buffer) - 1);
//     if (n > 0) {
//         buffer[n] = '\0';
//         ESP_LOGI(TAG, "received: %s", buffer);
//     } else {
//         ESP_LOGI(TAG, "read returned: %d", static_cast<int>(n));
//     }

//     link.close();
// }


static void process_message_line(const std::string& line) {
    if (line.empty()) {
        return;
    }

    ESP_LOGI(TAG, "message: %s", line.c_str());

    // Тут следующим шагом будет:
    // 1. parse JSON
    // 2. проверить msgId == "joystick.input"
    // 3. достать x/y
    // 4. пробросить во внутреннее сообщение Raven
}


static void consume_stream_chunk(std::string& rxBuffer, const char* data, size_t len) {
    if (data == nullptr || len == 0) {
        return;
    }

    rxBuffer.append(data, len);

    size_t newlinePos = 0;
    while ((newlinePos = rxBuffer.find('\n')) != std::string::npos) {
        std::string line = rxBuffer.substr(0, newlinePos);

        // убрать \r если вдруг прилетело \r\n
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        process_message_line(line);

        rxBuffer.erase(0, newlinePos + 1);
    }
}

// extern "C" void app_main() {
//     // Wi-Fi init у тебя уже есть и работает
//     // дождался IP, сеть поднята

//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     if (!wifi_init_sta()) {
//         ESP_LOGE(TAG, "wifi init failed");
//         return;
//     }

//     TcpServer server(8080);

//     if (!server.start()) {
//         ESP_LOGE("main", "server start failed");
//         while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
//     }

//     ESP_LOGI("main", "server listening on 8080");

//     while (true) {
//         auto client = server.acceptConnection();
//         if (!client) {
//             ESP_LOGE("main", "accept failed");
//             continue;
//         }

//         ESP_LOGI("main", "client connected");

//         std::string rxBuffer;

//         while (client->isOpen()) {  
//             char buffer[256] = {};
//             const ssize_t n = client->read(buffer, sizeof(buffer));

//             if (n <= 0) {
//                 ESP_LOGI("main", "client disconnected");
//                 break;
//             }

//             consume_stream_chunk(rxBuffer, buffer, static_cast<size_t>(n));
//         }

//         client->close();
//     }
// }

//extern "C" void app_main() {
//     // Wi-Fi init у тебя уже есть и работает
//     // дождался IP, сеть поднята

//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     if (!wifi_init_sta()) {
//         ESP_LOGE(TAG, "wifi init failed");
//         return;
//     }

//     TcpServer server(8080);

//     if (!server.start()) {
//         ESP_LOGE("main", "server start failed");
//         while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
//     }

//     ESP_LOGI("main", "server listening on 8080");

//     while (true) {
//         auto client = server.acceptConnection();
//         if (!client) {
//             ESP_LOGE("main", "accept failed");
//             continue;
//         }

//         ESP_LOGI("main", "client connected");

//         std::string rxBuffer;

//         while (client->isOpen()) {  
//             char buffer[256] = {};
//             const ssize_t n = client->read(buffer, sizeof(buffer));

//             if (n <= 0) {
//                 ESP_LOGI("main", "client disconnected");
//                 break;
//             }

//             consume_stream_chunk(rxBuffer, buffer, static_cast<size_t>(n));
//         }

//         client->close();
//     }
// }



namespace raven {


static FixedSizeDecoder joystick_decoder(msg_kind::PILOT_INPUT, sizeof(JoystickPayload));

static FixedSizeDecoder start_manual_decoder(msg_kind::NAV_CMD, sizeof(StartManualPayload));

static FixedSizeDecoder halt_manual_decoder(msg_kind::NAV_CMD, sizeof(HaltManualPayload));

static VariableSizeDecoder llm_response_decoder(msg_kind::LLM_DATA, 1, 4096);

extern "C" void app_main() {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "wifi init failed");
        return;
    }

    // Initialise the ESP-IDF default event loop — required by EventBus.
    raven::EventBus::init();

    // Shared navigation state — single source of truth for position.
    static raven::NavigationState nav_state;

    // Service: stub executor that updates state and signals completion.
    static raven::NavigationService nav_service(nav_state);
    nav_service.start();

    // Activity: owns behavior, reacts to directed messages.
    static raven::NavigationActivity nav_activity(nav_service);
    nav_activity.start();

    static TcpServer server(8080);

    static NetworkGateway gateway(
        server,
        NetworkGateway::Config{
            .name = "network_gateway",
            .stack_size = 4096,
            .priority = 5,
            .max_payload_size = 4096
        }
    );

        // Например:

    gateway.register_decoder(net_msg::JOYSTICK_INPUT, &joystick_decoder);
    gateway.register_decoder(net_msg::START_MANUAL_NAV, &start_manual_decoder);
    gateway.register_decoder(net_msg::HALT_MANUAL_NAV, &halt_manual_decoder);
    gateway.register_decoder(net_msg::LLM_RESPONSE_TEXT, &llm_response_decoder);

    // Routes:
    gateway.register_route(net_msg::JOYSTICK_INPUT, &nav_service);
    gateway.register_route(net_msg::START_MANUAL_NAV, &nav_activity);
    gateway.register_route(net_msg::HALT_MANUAL_NAV, &nav_activity);
    // gateway.register_route(net_msg::LLM_RESPONSE_TEXT, &llm_ingress_service);
    gateway.start();

}

}