#pragma once

// gateway_bootstrap.hpp — wiring helper for the navigation/pilot gateway example.
//
// Encapsulates static decoder instances and the registration of decoders and
// routes into NetworkGateway. app_main calls configure_navigation_gateway()
// once after constructing all long-lived objects.

#include "raven_gateways/network_gateway.hpp"
#include "raven_services/navigation_service.hpp"
#include "raven_services/pilot_input_service.hpp"
#include "raven_activities/navigation_activity.hpp"

namespace raven {

// Register all decoders and routes for the current navigation/pilot example.
// All registered decoder objects have static storage duration and outlive
// the gateway task.
void configure_navigation_gateway(
    NetworkGateway&      gateway,
    PilotInputService&   pilot_service,
    NavigationService&   nav_service,
    NavigationActivity&  nav_activity
);

} // namespace raven
