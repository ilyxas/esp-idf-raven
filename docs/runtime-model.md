# Raven — Runtime Model

This document describes the intended execution model for the Raven runtime: how FreeRTOS tasks are structured, how queues are used to deliver work, how events flow between components, and how shared-state snapshots are used to transfer data without tight coupling.

---

## Task Ownership

In the current foundational model, Activities are task-backed, and Services may own dedicated FreeRTOS tasks when independent asynchronous execution is justified.
**Controllers** own no tasks — they are passive, thread-safe data stores accessed by other tasks.

```
Task map (example)
──────────────────
app_main task          → Coordinator::bootstrap(), then deletes itself or suspends
BootActivity task      → processes queued events / commands related to the boot mode
IdleActivity task      → same pattern, only one activity task is "current" at a time
MotorService task      → drives motor hardware, applies commands, publishes state changes
CommsService task      → manages Wi-Fi / telemetry stack
SensorService task     → reads IMU / rangefinders, publishes sensor events
...
esp_event_loop task    → internal to ESP-IDF; delivers callbacks, must stay unblocked
```

### Why per-component tasks?

- A blocking call inside `SensorService` (e.g., waiting for an I²C transaction) does not stall `MotorService` or any activity.
- Each component processes its own queue at its own pace; priority and stack size can be tuned per component.
- There is no shared "main loop" that becomes a bottleneck.

---

## Per-Task Queues

For task-backed components, a task-owned queue is the preferred inbound coordination mechanism.
Cross-context work and directed commands should be enqueued and processed inside the owning task’s run loop.
```
Component lifecycle
───────────────────
start()
  └── xTaskCreate → task entry
        └── run loop:
              while (true) {
                  xQueueReceive(queue_, &msg, portMAX_DELAY);
                  dispatch(msg);   // business logic runs here, inside owner task
              }
```

**Key rule:** nothing that requires non-trivial logic runs directly in a callback or ISR. Work is always *posted* to the queue and *processed* in the task.

---

## Event Delivery Flow

The ESP-IDF event loop is used as the broadcast bus. The delivery path is:

```
1. Producer (Service or Activity) calls esp_event_post()
         │
         ▼
2. ESP-IDF event-loop task invokes registered callbacks
         │
         ▼
3. Callback (thin) — copies payload, posts message to receiver's queue
         │
         ▼
4. Receiver's run loop dequeues the message
         │
         ▼
5. Receiver's dispatch() executes the actual logic
```

### Why step 3 must be thin

The ESP-IDF event loop uses a single internal task to deliver all events. If any callback blocks, sleeps, or takes significant CPU time, *every* event delivery in the system is stalled. Callbacks must do nothing more than copy the payload and enqueue a message.

### Subscribing to events

A component registers a callback at startup (typically inside `Coordinator::bootstrap()` or the component's own `start()` method):

```cpp
esp_event_handler_register(RAVEN_SYSTEM_EVENTS, BATTERY_LOW,
                            &MotorService::on_battery_low_cb, this);

// Inside the callback:
static void on_battery_low_cb(void* arg, esp_event_base_t, int32_t, void* data) {
    auto* self = static_cast<MotorService*>(arg);
    BatteryLowEvent evt{};
    std::memcpy(&evt, data, sizeof(evt));
    // Post to own queue — do NOT act here
    self->post(Message{MSG_BATTERY_LOW, evt});
}
```

---

## Directed Command Flow

Commands are directed: they are addressed to a specific component.  
The sender posts a typed message directly to the target component's queue (or wraps it in a command event on the event bus if the sender does not hold a direct reference to the target).

```
Activity decides to stop motors
    │
    ▼
activity_.post_to(motor_service_, Command{CMD_HALT_MOTORS})
    │
    ▼
motor_service_ queue receives CMD_HALT_MOTORS
    │
    ▼
MotorService::dispatch() handles the command in its own task context
```

Direct queue posting is preferred when the sender already holds a reference to the target (avoids the overhead of the event loop for point-to-point messages).

---

## Shared-State Snapshot Flow

Shared state is never pushed to consumers. Consumers **pull a snapshot** at the moment they need it.

```
SensorService reads IMU hardware
    │
    ▼
SensorService calls sensor_state_.update(reading)   // thread-safe write
    │
    ▼
SensorService posts SENSOR_UPDATED event (no payload copy of full state needed)
    │
    ▼
NavActivity callback enqueues MSG_SENSOR_UPDATED
    │
    ▼
NavActivity run loop dequeues, calls sensor_state_.snapshot()  // thread-safe read
    │
    ▼
NavActivity processes snapshot in its own task context
```

### Snapshot semantics

- `update()` acquires a mutex, copies new data in, releases mutex.
- `snapshot()` acquires the same mutex, copies current data out, releases mutex.
- The snapshot is a plain value copy — the caller owns it and can use it freely without holding any lock.
- Event payloads are kept small (IDs and scalars, not full state blobs). Consumers retrieve the full state via `snapshot()` if needed.

---

## Coordinator Bootstrap Sequence

```
app_main()
  │
  └── Coordinator coordinator;
        │
        ├── 1. esp_event_loop_create_default()
        │
        ├── 2. Construct controllers
        │         sensor_state_, vehicle_state_, power_state_, comms_state_
        │
        ├── 3. Construct services (inject controller refs)
        │         motor_service_(&vehicle_state_)
        │         sensor_service_(&sensor_state_)
        │         comms_service_(&comms_state_)
        │         ...
        │
        ├── 4. Construct activities (inject controller + service refs)
        │         boot_activity_(&vehicle_state_, &motor_service_, ...)
        │         idle_activity_(...)
        │         ...
        │
        ├── 5. Register all event subscriptions
        │
        ├── 6. Start service tasks
        │
        └── 7. Start initial activity task (BootActivity)
```

The Coordinator owns all long-lived objects. Pointers to controllers and services are passed by reference at construction time — no globals, no singletons.

---

## Summary: Data Flow in One Sentence per Type

| Flow type | Direction | Mechanism |
|-----------|-----------|-----------|
| Broadcast event | one → many | `esp_event_post` → callbacks → receiver queues |
| Directed command | one → one | direct `xQueueSend` to target queue (or command event) |
| State write | service → controller | `controller.update(value)` under mutex |
| State read | consumer → controller | `controller.snapshot()` returns value copy |

---

*See also: [`architecture.md`](architecture.md) · [`event-model.md`](event-model.md)*
