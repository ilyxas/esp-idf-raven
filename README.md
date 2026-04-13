# esp-idf-raven

**Raven** is an ESP32-S3 based autonomous/manual navigation-capable vehicle platform designed for operation in uncertain or complex environments — underground tunnels, cave systems, mines, and open terrain alike.

This repository serves as the **architectural foundation** for the Raven runtime. It is not yet implementing the full navigation stack. Instead, it establishes the repository structure, runtime architecture, and the base abstractions that future services, activities, and subsystem logic will build upon.

---

## Architecture Overview

The Raven runtime is organized around four cooperating layers:

### Activities
High-level behavioral states of the vehicle, implemented as finite state machines (FSMs). Activities represent what the vehicle is currently *doing* — e.g., booting, standing by, navigating autonomously, or recovering from a fault. Activities orchestrate which services are active and react to system events to transition between modes.

### Services
Independent asynchronous workers, each owning its own FreeRTOS task. Services encapsulate interaction with hardware subsystems and background processing — motor control, sensor fusion, localization, telemetry, Wi-Fi, Bluetooth, lighting, and audio. Isolating services into separate tasks means that a blocking operation in one subsystem cannot stall the rest of the system.

### Controllers / Shared State
Thread-safe data containers that act as the **single source of truth** for system-wide state. Rather than scattering mutable state across services, canonical data (vehicle pose, motion state, power levels, sensor readings, connectivity status) lives in shared state objects protected by atomics or mutexes. Services write into shared state; Activities read from it to make decisions.

### Coordinator / Event Bus
The glue between layers, implemented using the ESP-IDF system event loop (`esp_event_loop`). Instead of direct cross-layer calls, subsystems post typed events (`WIFI_CONNECTED`, `OBSTACLE_DETECTED`, `BATTERY_LOW`, `MISSION_STARTED`, etc.) to the event bus. Other layers subscribe to the events they care about. This decoupling means new services and activities can be added without modifying existing code.

---

## Design Goals

- **Asynchronous subsystem work** — each service runs independently; slow I/O in one subsystem does not block others.
- **Event-driven coordination** — behavioral transitions and cross-subsystem notifications happen through a typed event bus, not direct coupling.
- **Single source of truth** — shared state is centralized and thread-safe; there is one canonical answer to "what is the current pose?", "is Wi‑Fi connected?", "what is the battery level?".
- **Scalability** — new sensors, services, and behavioral modes can be added without restructuring existing layers.

---

## Current Status

The repository is in the **foundational stage**:

- Project structure and build system in place (ESP-IDF, CMake).
- Architectural layers (Activities, Services, Controllers, Event Bus) are defined in concept and will be scaffolded incrementally.
- `main.c` is a minimal placeholder; no domain logic is implemented yet.

---

## Next Steps

- [ ] Define base `Service` class with FreeRTOS task lifecycle (`start`, `stop`, `run` loop).
- [ ] Define base `Activity` / FSM class with event subscription helpers.
- [ ] Implement initial shared state structures (`VehicleState`, `PowerState`, `CommsState`).
- [ ] Register core event IDs and event base definitions.
- [ ] Implement `BootActivity` and `StandbyActivity` as the first behavioral states.
- [ ] Integrate first hardware service (e.g., `MotorService` or `BatteryService`).
- [ ] Add `Coordinator` to wire activities, services, and the event bus at startup.

---

## Hardware Target

- **MCU:** ESP32-S3
- **Subsystems (planned):** motors/drive, IMU, rangefinders, lighting, audio, Wi-Fi, Bluetooth, telemetry

---

## Building

This project uses [ESP-IDF](https://github.com/espressif/esp-idf). Follow the [Getting Started guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) to set up the toolchain, then:

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```
