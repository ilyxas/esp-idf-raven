#pragma once

#include "raven_activities/base_activity.hpp"
#include "raven_services/navigation_service.hpp"

#include "esp_event.h"

namespace raven {

// NavigationActivity — reference activity demonstrating the Raven communication model.
//
// Owns an Idle/Working internal state machine. On receiving a MoveForward
// directed message it transitions to Working and delegates execution to
// NavigationService. It subscribes to the service completion event; the
// handler is intentionally thin and only posts a self-message. The
// transition back to Idle runs in this activity's own task context.
class NavigationActivity : public BaseActivity {
public:
    explicit NavigationActivity(NavigationService& nav_service);

protected:
    // Subscribes to NavigationService completion events.
    void on_start() override;

    // Dispatches incoming directed messages and self-messages.
    void handle_message(const TaskMessage& msg) override;

private:
    enum class State { Idle, Working, Manual };

    void handle_move_forward();
    void handle_move_done();
    void handle_start_manual_nav();
    void handle_halt_manual_nav();

    // Thin EventBus callback — translates the event into a self-message only.
    static void on_nav_event(void* ctx, esp_event_base_t base,
                             int32_t id, void* event_data);

    NavigationService& nav_service_;
    State              state_{State::Idle};
};

} // namespace raven
