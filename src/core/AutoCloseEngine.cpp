#include "AutoCloseEngine.h"

#include "app/config.h"
#include "app/globals.h"

AutoCloseEngine gAutoCloseEngine;

// Backward-compatible reference bindings for existing callers
uint32_t& gPendingCloseAtMs = gAutoCloseEngine.pendingCloseAtMsRef();
uint32_t& gSafetyCloseAtMs = gAutoCloseEngine.safetyCloseAtMsRef();
bool& gAutoCloseArmed = gAutoCloseEngine.autoCloseArmedRef();
bool& gAutoCloseAborted = gAutoCloseEngine.autoCloseAbortedRef();

void AutoCloseEngine::armSafetyClose(uint32_t targetCloseAtMs) {
  _safetyCloseAtMs = targetCloseAtMs;
  if (_safetyCloseAtMs == 0) _safetyCloseAtMs = 1;
  _pendingCloseAtMs = 0; // Safety sensor takes primary authority
}

void AutoCloseEngine::disarmSafetyClose() {
  _safetyCloseAtMs = 0;
}

void AutoCloseEngine::armPendingClose(uint32_t targetCloseAtMs) {
  _pendingCloseAtMs = targetCloseAtMs;
  _autoCloseArmed = true;
}

void AutoCloseEngine::disarmPendingClose() {
  _pendingCloseAtMs = 0;
  _autoCloseArmed = false;
}

void AutoCloseEngine::disarmAll() {
  _safetyCloseAtMs = 0;
  _pendingCloseAtMs = 0;
  _autoCloseArmed = false;
}

void AutoCloseEngine::tripCircuitBreaker() {
  _autoCloseAborted = true;
  _pendingCloseAtMs = 0;
  _safetyCloseAtMs = 0;
}

void AutoCloseEngine::resetCircuitBreaker() {
  _autoCloseAborted = false;
}

void AutoCloseEngine::update(const AutoCloseContext& ctx, IDoorExecutor* executor) {
  if (ctx.isSafeMode || !ctx.isTagAutoEnabled || ctx.isManualOpenState || _autoCloseAborted) {
    return;
  }

  const uint32_t now = ctx.now;

  // ── PRIORITY 1: Safety Sensor Auto-Close (Primary authority when passage occurred) ──
  // Bypasses 250ms rate limit to guarantee passage-close fires reliably.
  if (ctx.isSafetySensorEnabled && _safetyCloseAtMs != 0) {
    if ((int32_t)(now - _safetyCloseAtMs) >= 0) {
      if (!ctx.isSafetySensorBlocked && (ctx.doorState == DoorState::Open || ctx.doorState == DoorState::Opening || ctx.doorState == DoorState::Stopped)) {
        Serial.println("[safety] auto-closing door after passage completed");
        logDiagEvent(DoorEventCode::SafeClose,
                     DoorEventLevel::Success,
                     DiagReasonCode::AutoCloseSafety,
                     (uint16_t)gRelayPulseMs,
                     0,
                     "SAFETY");
        // Force-clear the rate-limit guard so Safety Close is never deferred by it
        gLastDoorAutoTxMs = 0;
        if (executor && executor->executeSlot("close", false, false, false /* standard Pre-Stop & Deadband when moving/opening */)) {
          _safetyCloseAtMs = 0;
          // Note: passageCrossed is maintained until door is actively Closing in noteClosingStarted()
          // to ensure safety close authority is preserved if deadband is pending or temporarily deferred.
        } else {
          _safetyCloseAtMs = now + 200; // Very short retry — safety is critical
        }
      } else if (ctx.isSafetySensorBlocked) {
        _safetyCloseAtMs = 0; // Will re-arm on next beam-clear event
      } else if (ctx.doorState == DoorState::Closing) {
        // Already closing — safety close satisfied, just clear the timer
        _safetyCloseAtMs = 0;
        if (_onPassageReset) {
          _onPassageReset();
        }
      }
    }
  }

  // ── PRIORITY 2: Normal Tag Departure & General Fallback Auto-Close ──
  // Arms if door is Open or Stopped, was not armed from open trigger, tags are inactive, and no close timer or reversal is active.
  if (!_autoCloseArmed && (ctx.doorState == DoorState::Open || ctx.doorState == DoorState::Stopped) &&
      !ctx.anyActiveNonSuppressed && _safetyCloseAtMs == 0 && _pendingCloseAtMs == 0 && !ctx.isReversing) {
    uint32_t delayMs = (ctx.fallbackDelayMs > 0) ? ctx.fallbackDelayMs : kDefaultTagCloseDelayMs;
    _pendingCloseAtMs = now + delayMs;
    _autoCloseArmed = true;
    Serial.printf("[door] schedule close in %lu ms (door %s, tags clear, fallback active)\n",
                  (unsigned long)delayMs, ctx.doorState == DoorState::Stopped ? "stopped" : "open");
  }

  // When pending close timer expires (Countdown reached 00:00)
  if (_pendingCloseAtMs != 0 && (int32_t)(now - _pendingCloseAtMs) >= 0) {
    if (ctx.doorState == DoorState::Opening) {
      // Do NOT auto-close while motor is actively in opening travel!
      return;
    }
    if (ctx.isSafetySensorEnabled && ctx.isSafetySensorBlocked) {
      _pendingCloseAtMs = now + 1000; // Defer if beam is blocked
      logDiagEvent(DoorEventCode::CloseBlock,
                   DoorEventLevel::Warn,
                   DiagReasonCode::BeamStillBlocked,
                   1,
                   0,
                   "SAFETY");
    } else if (!ctx.anyActiveNonSuppressed) {
      if (_safetyCloseAtMs != 0) {
        _pendingCloseAtMs = 0; // Safety close took priority
      } else {
        Serial.println("[door] auto-closing (countdown reached 00:00) -> executing single-round CLOSE");
        logDiagEvent(DoorEventCode::FallClose,
                     DoorEventLevel::Info,
                     DiagReasonCode::AutoCloseFallback,
                     (uint16_t)gRelayPulseMs,
                     0,
                     "AUTO");
        if (executor && executor->executeSlot("close", false, false, false)) {
          _pendingCloseAtMs = 0;
          if (_onFallbackSuccess) {
            _onFallbackSuccess();
          }
        } else {
          _pendingCloseAtMs = now + 300;
        }
      }
    } else {
      _pendingCloseAtMs = 0; // Active tag returned, cancel close
      _autoCloseArmed = false;
    }
  }

  // ── PRIORITY 3: Failsafe Max Open Timeout Protection (Max 3 Minutes) ──
  // If the door has been Open or Stopped for > 3 minutes (180s) without active tags (or passage already crossed), force auto-close for safety
  // One-shot cooldown: maximum one failsafe attempt per 5 seconds to prevent log flooding
  // when executeSlot triggers a deadband that keeps the door in Stopped state.
  const uint32_t configuredHoldMs = (ctx.fallbackDelayMs > 0) ? ctx.fallbackDelayMs : kDefaultTagCloseDelayMs;
  const uint32_t failsafeTimeoutMs = max((uint32_t)180000UL, configuredHoldMs + 30000UL);
  const bool failsafePassage = (ctx.hasPassageCrossed && ctx.isSafetySensorEnabled);
  if ((ctx.doorState == DoorState::Open || ctx.doorState == DoorState::Stopped) && ctx.doorOpenedAtMs != 0 &&
      (uint32_t)(now - ctx.doorOpenedAtMs) > failsafeTimeoutMs && (!ctx.anyActiveNonSuppressed || failsafePassage)) {
    if (!(ctx.isSafetySensorEnabled && ctx.isSafetySensorBlocked)) {
      if ((uint32_t)(now - _lastFailsafeAtMs) >= 5000) {
        _lastFailsafeAtMs = now;
        Serial.println("[door] failsafe timeout (3 mins open/stopped) -> initiating safe close");
        logDiagEvent(DoorEventCode::Failsafe,
                     DoorEventLevel::Warn,
                     DiagReasonCode::FailsafeOvertime,
                     180,
                     0,
                     "SAFETY");
        if (executor) {
          executor->executeSlot("close", false, false, false);
        }
      }
    }
  }
}
