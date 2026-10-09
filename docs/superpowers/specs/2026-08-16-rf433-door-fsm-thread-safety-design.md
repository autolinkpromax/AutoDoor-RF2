# RF433 Priority Normalization, Sequential Door FSM & Thread Safety Design

## Overview
This design document defines the architecture to:
1. **Normalize RF433 Priority**: Adjust the FreeRTOS task priority of the RF433 receiver to match standard tasks (Priority 1) and treat RF tags as first-class citizens in the tag pipeline rather than pre-empting the main loop with immediate hardware relay triggers.
2. **Sequential Door State Machine (FSM)**: Replace fragmented timer/flag logic with a deterministic finite state machine (`IDLE_CLOSED`, `OPENING`, `OPEN_HOLD`, `CLOSING_SEQUENCE`, `CLOSING`, `STOPPED`, `SAFETY_REVERSE`) that guarantees auto-close countdown execution and prevents doors from getting stuck open.
3. **Thread Safety & Command Dispatching**: Implement a centralized, thread-safe command queue / dispatcher so all door triggers (Web API, Blynk, Schedules, RF433, Auto-tags, Physical button) are processed deterministically in `loopTask` without concurrent race conditions or relay pin corruption.

---

## 1. System Architecture & Components

```
+--------------------------------------------------------------------------------+
|                               TRIGGER SOURCES                                  |
|  +--------------+  +--------------+  +-------------+  +-----------+  +-------+ |
|  | RF433 Task   |  | ESP-NOW Recv |  | Web / HTTP  |  | Blynk App |  | Sched | |
|  | (Priority 1) |  | (WiFi task)  |  | (Async/Web) |  | Service   |  | Timer | |
|  +-------+------+  +-------+------+  +------+------+  +-----+-----+  +---+---+ |
+----------|-----------------|----------------|---------------|------------|-----+
           |                 |                |               |            |
           | noteTagSeen()   | noteTagSeen()  | DoorCmd       | DoorCmd    | DoorCmd
           v                 v                |               |            |
+----------------------------------+          |               |            |
|       TAG STORE & REGISTRY       |          |               |            |
|   (Thread-Safe Mutex Lock)       |          |               |            |
+-----------------+----------------+          |               |            |
                  |                           |               |            |
                  | Active Tag Status         v               v            v
                  |               +----------------------------------------+---+
                  |               |       DOOR COMMAND QUEUE (FreeRTOS)        |
                  |               +--------------------+-----------------------+
                  |                                    |
                  v                                    v
+--------------------------------------------------------------------------------+
|                     SEQUENTIAL DOOR STATE MACHINE (FSM)                        |
|                               (loopTask)                                       |
|                                                                                |
|   [CLOSED] <----------+                                                        |
|      |                | (Close motion finishes / Mag sensor)                   |
|      | (Open Trigger) |                                                        |
|      v                |                                                        |
|   [OPENING] --------> [OPEN_HOLD]                                              |
|  (Move open, 5s)   (Hold while tag active / Countdown auto-close delay)        |
|                           |                                                    |
|                           | (Auto-close timer expires / Manual Close)          |
|                           v                                                    |
|                   [CLOSING_SEQUENCE]                                           |
|                   - Step 1: Pulse STOP #1 (500ms + 350ms)                      |
|                   - Step 2: Pulse STOP #2 (500ms + 350ms)                      |
|                   - Step 3: Pulse CLOSE -> Transition to [CLOSING]             |
|                           |                                                    |
|                           v                                                    |
|                       [CLOSING] -------------------------> [SAFETY_REVERSE]    |
|                  (Move close, 5s)     (Obstacle detected)  - Stop pulse        |
|                           |                                - 400ms deadband    |
|                           +------------------------------> - Reverse to OPEN   |
+--------------------------------------------------------------------------------+
                                           |
                                           v
                             +---------------------------+
                             |   RELAY HARDWARE DRIVER   |
                             |  (Atomic / Critical Sec)  |
                             +---------------------------+
```

---

## 2. Detailed Component Design

### 2.1 RF433 Priority & Tag Normalization (`rf433_runtime.cpp`)
- **Task Priority**: Lower `rf433Task` priority from 5 to 1 (same as `loopTask`).
- **Processing Flow**:
  - In `processRfCode(value)`:
    - If in Learn Mode: Store candidate under `TagStoreLock`.
    - If in Normal Mode:
      - Look up tag in `findTagByRfCode(value)`.
      - If tag exists and is valid:
        - Call `noteTagSeen(*tag, -50, now);` under `TagStoreLock`.
        - Enqueue a tag-open action or let `stepDoorLogic` pick up the active tag.
        - Add burst debounce (500ms) to ignore repeat frames from the same press.
        - **Remove direct `executeDoorSlot("open", true, false)` call from the RF task** to eliminate task preemption race conditions.
  - **Remove RF Hacks**:
    - Remove hardcoded 5000ms overrides in `tag_store.cpp` and `door.cpp`.
    - RF tags use the same `tagHoldMs` and `tagCloseDelayMs` configuration pipeline as all tags.

### 2.2 Door Sequential State Machine (`door.cpp`)
- **Explicit States**:
  - `DoorState::Closed`: Gate is idle and closed.
  - `DoorState::Opening`: Relay pulsed open; transition timer (`gDoorTransitionMs`) is ticking down or waiting for open limit switch.
  - `DoorState::Open`: Gate is fully open.
    - If any tag is active: Reset / hold the close countdown.
    - Once all tags are inactive and `gIsManualOpenState == false`: Arm `gPendingCloseAtMs = now + closeDelayMs`.
    - If `gPendingCloseAtMs` reaches `now`: Automatically initiate Close Sequence.
  - `DoorState::Closing`: Executing multi-pulse sequence:
    - Step 1: `DoorSeq::Stop1` -> Pulse STOP relay
    - Step 2: `DoorSeq::Stop2` -> Pulse STOP relay
    - Step 3: `DoorSeq::ClosePulse` -> Pulse CLOSE relay -> mark state as `Closing` with transition timer.
  - `DoorState::Stopped`: Gate stopped midway.
  - `DoorState::SafetyReversing`: Safety beam broken during closing -> Pulse STOP -> wait 400ms deadband -> transition to `Opening`.

- **Preventing "Door Stuck Open"**:
  1. **Guaranteed Arming on Entering `Open`**: When transitioning from `Opening` -> `Open`, if no tags are active and not in manual hold mode, `gPendingCloseAtMs` is immediately armed.
  2. **Active Tag Departure Check**: When tags transition from active to inactive during `Open`, `gPendingCloseAtMs` is armed using the longest active tag's `tagCloseDelayMs`.
  3. **Vehicle Passage Integration**: If the vehicle sensor detects a passage while in `Open` or `Opening`, it schedules close in `gSafetyCloseDelayMs` without being cancelled by stray tag events.
  4. **Failsafe Maximum Open Duration**: If the door remains in `Open` for > 5 minutes without active tags or manual hold, a failsafe force-close is executed.

### 2.3 Thread-Safe Command Dispatching
- **Command Structure**:
  ```cpp
  enum class DoorCmdAction : uint8_t { Open, Close, Stop };
  struct DoorCommand {
    DoorCmdAction action;
    bool isManual;
    bool manualHold;
  };
  ```
- **Queue**: FreeRTOS `QueueHandle_t gDoorCmdQueue` (capacity: 16 commands).
- **Enqueuing**:
  - `requestDoorSlot(slot, isManual, manualHold)`: Called from Web Server, Blynk, Schedule, RF433, or button. Safe to call from any task context.
- **Consumption**:
  - `stepDoorLogic()` in `loopTask` consumes commands from `gDoorCmdQueue` at the start of each iteration under `TagStoreLock`.

---

## 3. Error Handling & Edge Cases
- **Command while Closing**: If an `Open` command arrives during `Closing` or `ClosingSequence`, immediately cancel the sequence, pulse `Open`, and transition to `Opening`.
- **Command while Opening**: If a `Close` command arrives during `Opening`, stop opening and begin `ClosingSequence`.
- **Obstacle / Safety Sensor Triggered**: If beam breaks while in `Closing`, instantly abort close sequence, pulse STOP, and enter `SafetyReversing` (400ms delay before reversing to OPEN to protect motor gearbox).
- **Limit Switches & Ground Truth**: Magnetic sensor or built-in limit switches synchronize `Closed` or `Open` states immediately.

---

## 4. Verification Plan
1. **Compilation**: Verify build succeeds under PlatformIO `esp32-c3-release` and `esp32-c3-devkitm-1`.
2. **RF433 Triggering**: Verify RF remote code is received at priority 1, registers in tag store, and opens door cleanly.
3. **Sequential Auto-Close Flow**: Verify door transitions through `Opening` -> `Open` -> countdown expires -> `Stop1` -> `Stop2` -> `Close` -> `Closed`.
4. **Safety Sensor Obstacle**: Verify safety beam obstruction during closing stops the gate, waits 400ms, and reverses to open.
5. **No Stuck Open**: Verify tag entering and leaving always causes auto-close timer to arm and close the gate.
