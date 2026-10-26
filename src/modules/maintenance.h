#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "core/IDoorListener.h"

/**
 * @brief Gate Lifetime Cycle Counter & Predictive Maintenance Reminder Subsystem.
 * 
 * Tracks total lifetime door operations (odometer) and cycles since last mechanical service.
 * Triggers alerts when reaching configured threshold (e.g., 5,000 cycles) to remind users
 * to inspect chain tension, clean photocell lenses, and lubricate guide tracks/rollers.
 */
class MaintenanceManager : public IDoorListener {
public:
  MaintenanceManager() = default;

  void begin(Preferences& prefs);

  // ── IDoorListener Implementation ──
  void onDoorStateChanged(DoorState oldState, DoorState newState) override {}
  void onDoorClosed() override;

  // ── Odometer & Service Operations ──
  void incrementCycle();
  void resetService(const String& dateStr);
  void setThreshold(uint32_t threshold);
  void setTotalCycles(uint32_t count);

  uint32_t getTotalCycles() const { return _totalCycles; }
  uint32_t getServiceCycles() const { return _serviceCycles; }
  uint32_t getServiceThreshold() const { return _serviceThreshold; }
  bool isServiceDue() const { return _serviceCycles >= _serviceThreshold; }
  const String& getLastServiceDate() const { return _lastServiceDate; }

private:
  void save();

  Preferences* _prefs{nullptr};
  uint32_t _totalCycles{0};
  uint32_t _serviceCycles{0};
  uint32_t _serviceThreshold{5000};
  String _lastServiceDate{""};
};

extern MaintenanceManager gMaintenance;
