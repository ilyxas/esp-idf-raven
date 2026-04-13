idf_component_register(
    SRCS "src/raven_services.cpp"
    INCLUDE_DIRS "include"
    REQUIRES raven_core raven_events raven_state
)
