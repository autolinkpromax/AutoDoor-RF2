# RF433 Priority Normalization, Sequential Door FSM & Thread Safety Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Equalize RF433 receiver priority with other tasks, implement a robust sequential finite state machine (FSM) for door open/close to eliminate door stuck-open conditions, and provide thread-safe command dispatching.

**Architecture:** Create a centralized FreeRTOS door command queue, normalize `rf433Task` priority to 1, unify RF and ESP-NOW tag lifecycle management, and structure `door.cpp` into a deterministic FSM with guaranteed auto-close arming and multi-pulse sequential closing.

**Tech Stack:** ESP32-C3 (Single-Core RISC-V), Arduino framework, FreeRTOS queues & mutexes, PlatformIO.

---

### Task 1: Door Command Types and Thread-Safe Interface

**Files:**
- Modify: `src/app/types.h`
- Modify: `src/modules/door.h`

- [ ] **Step 1: Define DoorCommand enum and struct in `types.h`**

Add `DoorCmdAction` enum and `DoorCommand` struct to `src/app/types.h`:
```cpp
enum class DoorCmdAction : uint8_t {
  Open = 1,
  Close = 2,
  Stop = 3
};

struct DoorCommand {
  DoorCmdAction action;
  bool isManual;
  bool manualHold;
};
```

- [ ] **Step 2: Declare `requestDoorSlot` and command queue initialization in `door.h`**

Update `src/modules/door.h` to declare:
```cpp
void initDoorCommandQueue();
bool requestDoorSlot(const String& slot, bool manualCommand, bool manualHold = true);
bool requestDoorAction(DoorCmdAction action, bool manualCommand, bool manualHold = true);
```

- [ ] **Step 3: Commit interface definitions**

```bash
git add src/app/types.h src/modules/door.h
git commit -m "feat: declare DoorCommand types and thread-safe requestDoorSlot interface"
```

---

### Task 2: RF433 Priority Normalization & Tag Unification

**Files:**
- Modify: `src/modules/rf433_runtime.cpp`
- Modify: `src/modules/tag_store.cpp`
- Modify: `src/modules/tag_store.h`

- [ ] **Step 1: Lower `rf433Task` priority from 5 to 1 in `rf433_runtime.cpp`**

In `initRf433()`, change task priority from `5` to `1`:
```cpp
xTaskCreate(rf433Task, "rf433Task", 4096, NULL, 1, NULL);
```

- [ ] **Step 2: Route RF tag detection through `noteTagSeen` and `requestDoorAction`**

In `processRfCode(uint32_t value)` in `rf433_runtime.cpp`:
- Inside `gTagStoreMutex`:
  - Find tag with `findTagByRfCode(value)`.
  - When found:
    - Mark tag seen via `noteTagSeen(*tag, -50, now);`.
    - Apply 500ms remote debounce.
    - Call `requestDoorAction(DoorCmdAction::Open, true, false);` to queue the command safely instead of pulsing hardware directly.
    - Remove direct calls to `pulseRelay` / `executeDoorSlot`.

- [ ] **Step 3: Remove RF-specific hardcoded 5000ms overrides in `tag_store.cpp`**

In `tag_store.cpp`:
- In `isTagActive()`: Use `t.tagHoldMs` consistently for all tags (`uint32_t holdMs = (t.tagHoldMs > 0) ? t.tagHoldMs : 5000;`).
- In `createTag()` and `updateTag()`: Allow RF tags to have customizable `tagHoldMs` and `tagCloseDelayMs`.
- In `loadTagsFromPrefs()`: Do not hardcode 5000ms over stored preferences if a valid hold is set.

- [ ] **Step 4: Commit RF433 normalization**

```bash
git add src/modules/rf433_runtime.cpp src/modules/tag_store.cpp src/modules/tag_store.h
git commit -m "refactor: normalize rf433 task priority and unify tag store behavior"
```

---

### Task 3: Refactor Sequential Door State Machine & Auto-Close Safeguards

**Files:**
- Modify: `src/modules/door.cpp`

- [ ] **Step 1: Implement `gDoorCmdQueue` and queue processor**

In `src/modules/door.cpp`:
- Create `QueueHandle_t gDoorCmdQueue = NULL;` with size 16.
- Implement `initDoorCommandQueue()`.
- Implement `requestDoorAction()` to post to `gDoorCmdQueue` using `xQueueSend(gDoorCmdQueue, &cmd, 0)`.
- Implement `requestDoorSlot()` wrapper.

- [ ] **Step 2: Refactor door execution into deterministic sequential steps**

Implement sequential FSM in `door.cpp`:
- `executeDoorAction(DoorCmdAction action, bool isManual, bool manualHold)`:
  - If `Open`: cancel any pending close or closing sequence, trigger relay open pulse, set `gDoorState = DoorState::Opening`, arm transition timer `gDoorTransitionEndMs = now + gDoorTransitionMs`, set `gIsManualOpenState = (isManual && manualHold)`.
  - If `Close`: cancel open/pending timers, begin close sequence (`DoorSequenceStep::Close_Stop1`), set `gDoorState = DoorState::Closing`.
  - If `Stop`: begin stop sequence (`DoorSequenceStep::Stop_Stop1`), set `gDoorState = DoorState::Stopped`.

- [ ] **Step 3: Eliminate Door-Stuck-Open in `stepDoorLogic()`**

In `stepDoorLogic()`:
- Process queued commands from `gDoorCmdQueue`.
- Ground truth sensor sync (Mag sensor / Limit switches).
- Safety sensor beam monitoring & emergency stop with 400ms deadband (`gSafetyReverseAtMs`).
- When `gDoorState == DoorState::Opening` and `gDoorTransitionEndMs` reaches `now`:
  - Transition state to `DoorState::Open`.
  - If no active tags and `!gIsManualOpenState`, **immediately arm `gPendingCloseAtMs = now + closeDelayMs`**.
- In `DoorState::Open`:
  - If active tags exist: keep `gPendingCloseAtMs = 0`.
  - Once active tags clear: arm `gPendingCloseAtMs = now + max(1000, activeDelayMax)`.
  - If `gPendingCloseAtMs` expires: execute `DoorCmdAction::Close`.
- Multi-pulse closing sequence execution (`Close_Stop1` -> `Close_Stop2` -> `Close_Pulse` -> `Closing`).
- Failsafe watchdog: If open for > 5 minutes without active tags, force close.
- Remove outdated RF hacks (`isRfTagActiveOrPendingClose` replaced by unified tag checks).

- [ ] **Step 4: Commit door FSM refactoring**

```bash
git add src/modules/door.cpp src/modules/door.h
git commit -m "feat: implement sequential door FSM, thread-safe queue and auto-close guarantees"
```

---

### Task 4: Update External Trigger Sources & Callers

**Files:**
- Modify: `src/modules/http_api.cpp`
- Modify: `src/modules/blynk_service.cpp`
- Modify: `src/modules/schedule.cpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: Update `http_api.cpp` to call `requestDoorSlot`**
- [ ] **Step 2: Update `blynk_service.cpp` to call `requestDoorSlot`**
- [ ] **Step 3: Update `schedule.cpp` to call `requestDoorSlot`**
- [ ] **Step 4: Initialize door command queue in `main.cpp` setup**
- [ ] **Step 5: Commit caller updates**

```bash
git add src/modules/http_api.cpp src/modules/blynk_service.cpp src/modules/schedule.cpp src/main.cpp
git commit -m "refactor: use thread-safe requestDoorSlot across all external triggers"
```

---

### Task 5: Build & Verification

**Files:**
- None (Build verification)

- [ ] **Step 1: Run build for `esp32-c3-release`**
```bash
pio run -e esp32-c3-release
```
Expected: SUCCESS

- [ ] **Step 2: Run build for `esp32-c3-devkitm-1`**
```bash
pio run -e esp32-c3-devkitm-1
```
Expected: SUCCESS

- [ ] **Step 3: Final sanity review of diff and commit**
```bash
git status
```
