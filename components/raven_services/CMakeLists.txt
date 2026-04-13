idf_component_register(
    SRCS "src/raven_services.c"
    INCLUDE_DIRS "include"
    REQUIRES raven_core raven_events raven_state
)
