# Raven — Event Model

This document defines the three categories of inter-component communication in the Raven runtime and the rules that govern each one.

---

## The Three Categories

| Category | Direction | Meaning | Mechanism |
|----------|-----------|---------|-----------|
| Broadcast event | one → many | "this fact occurred" | `esp_event_post` → all subscribers |
| Directed command | one → one | "you should do this" | direct queue post (or command event) |
| State read / write | any → controller | "what is X?" / "X is now Y" | `snapshot()` / `update()` on a controller |

These must not be conflated. Mixing them creates coupling and makes the system harder to reason about.

---

## Broadcast Events

### Definition

A broadcast event communicates a **fact** — something that has already happened. The producer does not know, and should not care, who is listening.

### Characteristics

- Delivered to every registered subscriber via the ESP-IDF event loop.
- Payload is a small, copyable struct (scalars and IDs, no pointers to heap).
- The producer is not waiting for any acknowledgement.
- Any number of subscribers may react, or none at all.

### Examples

```
BATTERY_LOW          — power level has fallen below threshold
OBSTACLE_DETECTED    — rangefinder returned distance < safe threshold
WIFI_CONNECTED       — Wi-Fi association completed successfully
WIFI_DISCONNECTED    — Wi-Fi link lost
TELEMETRY_TIMEOUT    — no uplink heartbeat for N seconds
SENSOR_UPDATED       — IMU / sensor data has been refreshed in shared state
MISSION_STARTED      — a new mission waypoint list has been loaded
FAULT_DETECTED       — a service has entered a fault state
```

### Naming convention

Broadcast events are named in the **past tense** — they describe what happened:  
`BATTERY_LOW`, `OBSTACLE_DETECTED`, `WIFI_CONNECTED`.

### Payload rules

- Payloads must be trivially copyable (`memcpy`-safe).
- No raw pointers. If a reference to a larger structure is needed, pass an ID or version number and let the subscriber read shared state via `snapshot()`.
- Keep payloads small — anything larger than ~32 bytes should be left in shared state.

### Event base separation

Events are grouped by origin into named event bases:

```
RAVEN_SYSTEM_EVENTS   — lifecycle, faults, global state transitions
RAVEN_SENSOR_EVENTS   — sensor data updates, threshold crossings
RAVEN_COMMS_EVENTS    — Wi-Fi, Bluetooth, telemetry link changes
RAVEN_MISSION_EVENTS  — mission load, start, waypoint, completion
RAVEN_POWER_EVENTS    — battery levels, charge state, power mode
```

Grouping by base allows subscribers to filter efficiently and makes event ownership obvious.

### Callback rule

ESP-IDF delivers callbacks on the event-loop task. **Callbacks must be thin** — copy the payload, post a message to the receiver's own queue, and return. No logic, no blocking, no allocation.

```cpp
// Correct
static void on_battery_low_cb(void* arg, esp_event_base_t, int32_t, void* data) {
    auto* self = static_cast<NavActivity*>(arg);
    BatteryLowEvent evt{};
    std::memcpy(&evt, data, sizeof(evt));
    self->post(Message{MSG_BATTERY_LOW, evt});   // enqueue, do not process
}

// Wrong — logic inside callback
static void on_battery_low_cb(void* arg, esp_event_base_t, int32_t, void* data) {
    auto* self = static_cast<NavActivity*>(arg);
    self->stop_navigation();    // ← executes on event-loop task, may block, may starve other events
}
```

---

## Directed Commands

### Definition

A directed command communicates **intent to a specific recipient**. The sender knows who should act. The recipient is required to handle it.

### Characteristics

- Addressed to exactly one component.
- Posted directly to the target component's inbound queue (preferred when the sender holds a reference to the target).
- Can alternatively be wrapped as a command event on the event bus when the sender does not hold a direct reference — but this should be an exception, not the norm.
- The sender does not block waiting for the result; commands are fire-and-enqueue.
- The recipient processes the command in its own task context.

### Examples

```
CMD_HALT_MOTORS         → MotorService
CMD_SET_SPEED           → MotorService  (payload: target velocity)
CMD_SET_LIGHT_PROFILE   → LightService  (payload: profile id)
CMD_START_TELEMETRY     → CommsService
CMD_STOP_TELEMETRY      → CommsService
CMD_ENTER_LOW_POWER     → PowerService
```

### Naming convention

Commands are named in the **imperative form** — they express what should be done:  
`CMD_HALT_MOTORS`, `CMD_SET_SPEED`, `CMD_START_TELEMETRY`.

### Command structs

```cpp
struct CmdHaltMotors {};

struct CmdSetSpeed {
    float target_mps;
    uint32_t ramp_ms;
};
```

### Dispatch pattern

The receiving component's run loop deserialises the message type and calls the appropriate handler inside the owner task:

```cpp
void MotorService::dispatch(const Message& msg) {
    switch (msg.type) {
        case CMD_HALT_MOTORS:    handle_halt();                           break;
        case CMD_SET_SPEED:      handle_set_speed(msg.as<CmdSetSpeed>()); break;
        case MSG_BATTERY_LOW:    handle_battery_low();                    break;
        default:
            ESP_LOGW(TAG, "MotorService: unhandled message type %d", msg.type);
            break;
    }
}
```

---

## State Reads and Writes

### Definition

State reads and writes are **synchronous, direct calls to a controller**. They are not events and do not travel through any bus.

### Write (update)

A service calls `controller.update(new_value)` after it has new data to record. The controller handles synchronisation internally.

```cpp
// Inside SensorService task
SensorReading r = read_imu();
sensor_state_.update(r);                    // thread-safe, blocking on mutex briefly
esp_event_post(RAVEN_SENSOR_EVENTS, SENSOR_UPDATED, nullptr, 0, 0);
```

### Read (snapshot)

Any component that needs current state calls `controller.snapshot()` and receives a **value copy**. The copy is made under the controller's lock; the caller is free to use it without holding any lock.

```cpp
// Inside NavActivity run loop, after receiving MSG_SENSOR_UPDATED
SensorReading r = sensor_state_.snapshot();    // thread-safe, returns copy
if (r.distance_front_mm < OBSTACLE_THRESHOLD_MM) {
    post_to(motor_service_, Message{CMD_HALT_MOTORS});
}
```

### Rules

- State is **pulled** by consumers on demand, never pushed.
- Event payloads are **not** used to carry large state blobs. Publish a notification event; let the subscriber call `snapshot()`.
- Controllers are never called from within an event callback — always from the owner task's run loop after dequeuing.

---

## Decision Guide: Which Category to Use?

```
"Something just happened and other subsystems may care"
    → Broadcast event (past tense, fire-and-forget)

"I need this specific component to take an action"
    → Directed command (imperative, to target queue)

"I need to know the current value of X"
    → State read: controller.snapshot()

"I have new authoritative data for X"
    → State write: controller.update(value)
       then optionally a broadcast event to notify interested parties
```

---

*See also: [`architecture.md`](architecture.md) · [`runtime-model.md`](runtime-model.md)*
