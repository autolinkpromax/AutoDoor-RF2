#include "globals.h"
#include "modules/door.h"

WebServer gServer(80);
Preferences gPrefs;

LearnState gLearn;

bool gPrevAnyTagActive = false;
uint32_t gPrevActiveDelayMaxMs = 0;
uint32_t gPrevActiveSafetyDelayMs = 0;
String gActiveTriggerTagId = "";

bool gApEnabled = true;
uint32_t gApLastHadStationsMs = 0;
uint32_t gLastDoorAutoTxMs = 0;

uint32_t gBootMs = 0;
bool gSystemBootReady = false;
uint32_t gWifiScanStartMs = 0;

bool gWifiStatic = false;
bool gTimeSynced = false;

std::vector<ScheduleEntry> gSchedules;
uint32_t gLastScheduleCheckMs = 0;

bool gBlynkEnabled = false;
bool gBlynkConnected = false;
String gBlynkToken;
String gBlynkTemplateId;
String gBlynkDeviceName;
char gBlynkAuthBuf[64] = {};

String gWebPassword;

bool gMagSensorStableClosed = false;
bool gBuiltinMagOpen = false;
bool gBuiltinMagClose = false;
bool gSafetySensorBlocked = false;
