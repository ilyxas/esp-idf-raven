#include "raven_activities/navigation_activity.hpp"
#include "raven_services/navigation_messages.hpp"
#include "raven_events/event_bus.hpp"
#include "raven_events/navigation_events.hpp"

#include "esp_log.h"

namespace raven {

static const char* TAG = "NavigationActivity";

NavigationActivity::NavigationActivity(NavigationService& nav_service)
    : BaseActivity({ "nav_activity", 4096, 5, 8 }), nav_service_(nav_service)
{
}

void NavigationActivity::on_start()
{
    // Subscribe to NavigationService completion events.
    // The callback remains thin: it only translates the event into a self-message.
    EventBus::subscribe(NAVIGATION_EVENTS, NAV_EVENT_MOVE_FORWARD_DONE,
                        &NavigationActivity::on_nav_event, this);

    ESP_LOGI(TAG, "started — state: Idle");
}

void NavigationActivity::handle_message(const TaskMessage& msg)
{
    if (msg.kind != nav_msg::KIND) {
        return;
    }

    switch (msg.id) {
        case nav_msg::MOVE_FORWARD: handle_move_forward(); break;
        case nav_msg::MOVE_DONE:    handle_move_done();    break;
        default:                                           break;
    }
}

void NavigationActivity::handle_move_forward()
{
    if (state_ != State::Idle) {
        ESP_LOGW(TAG, "MoveForward ignored — not Idle");
        return;
    }

    ESP_LOGI(TAG, "MoveForward — Idle → Working");
    state_ = State::Working;

    // Delegate execution to NavigationService via a directed message.
    nav_service_.post_message({ nav_msg::KIND, nav_msg::MOVE_FORWARD, nullptr });
}

void NavigationActivity::handle_move_done()
{
    // Business logic runs here, in the activity's own task context — not
    // inside the event callback.
    ESP_LOGI(TAG, "MoveDone — Working → Idle");
    state_ = State::Idle;
}

// static
void NavigationActivity::on_nav_event(void* ctx, esp_event_base_t /*base*/,
                                       int32_t /*id*/, void* /*event_data*/)
{
    // Thin callback: translate the completion event into a directed self-message
    // and return immediately. All business logic executes in handle_move_done().
    auto* self = static_cast<NavigationActivity*>(ctx);
    self->post_message({ nav_msg::KIND, nav_msg::MOVE_DONE, nullptr });
}

} // namespace raven
