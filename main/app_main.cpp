// app_main.cpp — application entrypoint.
//
// Demonstrates the minimal end-to-end Raven reference flow:
//   1. NavigationActivity receives a directed MoveForward message.
//   2. It transitions Idle → Working and posts the command to NavigationService.
//   3. NavigationService performs a stub position update and publishes a
//      completion event via EventBus.
//   4. The thin event callback translates the event into a self-message.
//   5. NavigationActivity processes the self-message in its own task context
//      and transitions Working → Idle.

#include "raven_events/event_bus.hpp"
#include "raven_state/navigation_state.hpp"
#include "raven_services/navigation_service.hpp"
#include "raven_services/navigation_messages.hpp"
#include "raven_activities/navigation_activity.hpp"

extern "C" void app_main(void)
{
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

    // Trigger the reference flow: send a MoveForward directed command.
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    nav_activity.post_message({ raven::nav_msg::KIND, raven::nav_msg::MOVE_FORWARD, nullptr });
    
}
