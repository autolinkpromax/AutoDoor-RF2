#include "maintenance.h"
#include "app/config.h"

MaintenanceManager gMaintenance;

void MaintenanceManager::begin(Preferences& prefs) {
  _prefs = &prefs;
  _totalCycles = _prefs->getUInt("maintTotal", 0);
  _serviceCycles = _prefs->getUInt("maintServ", 0);
  _serviceThreshold = _prefs->getUInt("maintThresh", 5000);
  if (_serviceThreshold == 0) _serviceThreshold = 5000;
  _lastServiceDate = _prefs->getString("maintDate", "");

  Serial.printf("[maint] Initialized. TotalCycles=%u, SinceService=%u, Threshold=%u, LastDate=%s, Due=%s\n",
                _totalCycles, _serviceCycles, _serviceThreshold,
                _lastServiceDate.isEmpty() ? "None" : _lastServiceDate.c_str(),
                isServiceDue() ? "YES" : "NO");
}

void MaintenanceManager::onDoorClosed() {
  incrementCycle();
}

void MaintenanceManager::incrementCycle() {
  _totalCycles++;
  _serviceCycles++;
  save();

  AD_LOG(Serial.printf("[maint] Door closed -> Cycle count: Total=%u, SinceService=%u (Threshold=%u)%s\n",
                       _totalCycles, _serviceCycles, _serviceThreshold,
                       isServiceDue() ? " [SERVICE DUE!]" : ""));
}

void MaintenanceManager::resetService(const String& dateStr) {
  _serviceCycles = 0;
  if (!dateStr.isEmpty()) {
    _lastServiceDate = dateStr;
  }
  save();
  Serial.printf("[maint] Service reminder reset. Date=%s, TotalCycles=%u\n",
                _lastServiceDate.c_str(), _totalCycles);
}

void MaintenanceManager::setThreshold(uint32_t threshold) {
  if (threshold < 100) threshold = 100;
  if (threshold > 100000) threshold = 100000;
  _serviceThreshold = threshold;
  save();
  Serial.printf("[maint] Service threshold set to %u cycles\n", _serviceThreshold);
}

void MaintenanceManager::setTotalCycles(uint32_t count) {
  _totalCycles = count;
  save();
}

void MaintenanceManager::save() {
  if (!_prefs) return;
  _prefs->putUInt("maintTotal", _totalCycles);
  _prefs->putUInt("maintServ", _serviceCycles);
  _prefs->putUInt("maintThresh", _serviceThreshold);
  _prefs->putString("maintDate", _lastServiceDate);
}
