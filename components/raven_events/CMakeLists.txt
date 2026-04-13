idf_component_register(
    SRCS "src/raven_events.cpp"
         "src/event_bus.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_event
)
