#pragma once

#include "esp_event.h"

// raven_events: event bus definitions.
// Declare ESP-IDF event bases and event IDs for the Raven runtime here.
// Events communicate facts; directed commands are posted to component queues.

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(RAVEN_EVENTS);

#ifdef __cplusplus
}
#endif
