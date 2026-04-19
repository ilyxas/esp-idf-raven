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
    gateway.register_route(net_msg::JOYSTICK_INPUT,   &nav_service);
    gateway.register_route(net_msg::START_MANUAL_NAV, &nav_activity);
    gateway.register_route(net_msg::HALT_MANUAL_NAV,  &nav_activity);
    // gateway.register_route(net_msg::LLM_RESPONSE_TEXT, &llm_ingress_service);
}

} // namespace raven
