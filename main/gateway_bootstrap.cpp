#include "gateway_bootstrap.hpp"

#include "raven_gateways/decoder.hpp"
#include "raven_services/pilot_messages.hpp"

namespace raven {

// Static decoder instances — must outlive the NetworkGateway task.
static FixedSizeDecoder   s_joystick_decoder(msg_kind::PILOT_INPUT, sizeof(JoystickPayload));
static FixedSizeDecoder   s_start_manual_decoder(msg_kind::NAV_CMD, sizeof(StartManualPayload));
static FixedSizeDecoder   s_halt_manual_decoder(msg_kind::NAV_CMD, sizeof(HaltManualPayload));
static VariableSizeDecoder s_llm_response_decoder(msg_kind::LLM_DATA, 1, 4096);

void configure_navigation_gateway(
    NetworkGateway&      gateway,
    PilotInputService&   pilot_service,
    NavigationService&   nav_service,
    NavigationActivity&  nav_activity
)
{
    // Register decoders: network header msg_id → payload validator.
    gateway.register_decoder(net_msg::JOYSTICK_INPUT,    &s_joystick_decoder);
    gateway.register_decoder(net_msg::START_MANUAL_NAV,  &s_start_manual_decoder);
    gateway.register_decoder(net_msg::HALT_MANUAL_NAV,   &s_halt_manual_decoder);
    gateway.register_decoder(net_msg::LLM_RESPONSE_TEXT, &s_llm_response_decoder);

    // Register routes: decoded msg_id → receiving task.
    // JOYSTICK_INPUT goes to PilotInputService which writes NavigationState.
    // NavigationActivity polls state during Manual mode via the tick mechanism.
    gateway.register_route(net_msg::JOYSTICK_INPUT,   &pilot_service);
    gateway.register_route(net_msg::START_MANUAL_NAV, &nav_activity);
    gateway.register_route(net_msg::HALT_MANUAL_NAV,  &nav_activity);
    // gateway.register_route(net_msg::LLM_RESPONSE_TEXT, &llm_ingress_service);

    (void)nav_service; // reserved for future internal-only navigation commands
}

} // namespace raven
