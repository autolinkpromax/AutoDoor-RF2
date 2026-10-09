const TAG_DEF = {
  openDurationSec: 0,
  holdSec: 20,
  rfHoldSec: 1,
  closeDelaySec: 10,
  rssiMin: -100,
  passRssi: -90,
  passCloseSec: 5,
  safetyCloseDelaySec: 3,
};

const DAY_LABELS = ['อา', 'จ', 'อ', 'พ', 'พฤ', 'ศ', 'ส'];

const RSSI_ZONE_LABEL = {
  approaching: 'กำลังเข้าใกล้',
  in_range: 'สัญญาณปกติ',
  departing: 'กำลังออก',
  out: 'นอกระยะ',
};

const CLOCK_SVG =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></svg>';

const TAG_SIG_SVG =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><circle cx="12" cy="12" r="2"/><path d="M16.24 7.76a6 6 0 0 1 0 8.49M7.76 7.76a6 6 0 0 0 0 8.49M19.07 4.93a10 10 0 0 1 0 14.14M4.93 4.93a10 10 0 0 0 0 14.14"/></svg>';

const RF_REMOTE_SVG =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="3" width="12" height="18" rx="3"/><circle cx="12" cy="8" r="1.5" fill="currentColor"/><circle cx="12" cy="13" r="1.5" fill="currentColor"/><path d="M12 17h.01"/></svg>';

const VEHICLE_ICONS = {
  suv: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 17h2c.6 0 1-.4 1-1v-3c0-.9-.7-1.7-1.5-1.9C18.7 10.6 16 10 16 10s-1.3-1.4-2.2-2.3c-.5-.4-1.1-.7-1.8-.7H5c-.6 0-1.1.4-1.4.9l-1.5 2.8C2 11 2 11.3 2 11.6V16c0 .6.4 1 1 1h2"/><circle cx="7" cy="17" r="2.5"/><circle cx="17" cy="17" r="2.5"/><path d="M5 11h11"/></svg>',
  sedan: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="1" y="5" width="15" height="11" rx="2"/><circle cx="5.5" cy="18.5" r="2.5"/><circle cx="17.5" cy="18.5" r="2.5"/><path d="M16 8h4l3 4v4h-7V8z"/></svg>',
  pickup: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 17h1m4 0h8m4 0h1a1 1 0 0 0 1-1v-4a2 2 0 0 0-2-2h-3l-2-4H5a2 2 0 0 0-2 2v8a1 1 0 0 0 1 1"/><circle cx="6" cy="17" r="2.5"/><circle cx="18" cy="17" r="2.5"/><path d="M11 6v5h10"/></svg>',
  bike: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="5.5" cy="17.5" r="3.5"/><circle cx="18.5" cy="17.5" r="3.5"/><path d="M15 6h2m-5 11.5l3-7.5h3.5M5.5 17.5l4-7.5h5"/></svg>',
  scooter: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="6" cy="18" r="3"/><circle cx="18" cy="18" r="3"/><path d="M18 15V7h-3"/><path d="M9 18h6"/><path d="M6 15l3-5h4"/></svg>',
  van: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="6" width="20" height="11" rx="2"/><circle cx="7" cy="17" r="2.5"/><circle cx="17" cy="17" r="2.5"/><path d="M2 12h20M9 6v6"/></svg>',
  rf: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="3" width="12" height="18" rx="3"/><circle cx="12" cy="8" r="1.5" fill="currentColor"/><circle cx="12" cy="13" r="1.5" fill="currentColor"/><path d="M12 17h.01"/></svg>',
  bicycle: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="5.5" cy="17.5" r="3.5"/><circle cx="18.5" cy="17.5" r="3.5"/><path d="M12 17.5V14l-3-3 4-3 2 3h3M12 14l3.5 3.5"/></svg>',
  person: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="5" r="3"/><path d="M12 8v8m-3 3l3-3 3 3m-6-10l3 2 3-2"/></svg>',
  auto: TAG_SIG_SVG,
};

const VEHICLE_EMOJIS = {
  suv: '🚙',
  sedan: '🚗',
  car: '🚗',
  pickup: '🚚',
  bike: '🏍️',
  scooter: '🛵',
  van: '🚐',
  rf: '🔑',
  bicycle: '🚲',
  person: '🚶',
  auto: '🏷️',
  web: '💻',
  app: '📱',
  schedule: '⏰',
  btn: '🔘',
  ext: '⚡',
};

function getVehicleEmoji(iconKey, isRf = false) {
  if (iconKey && iconKey !== 'auto' && VEHICLE_EMOJIS[iconKey]) {
    return VEHICLE_EMOJIS[iconKey];
  }
  return isRf ? '🔑' : '🏷️';
}

function getVehicleIconSvg(iconKey, isRf = false) {
  if (iconKey && iconKey !== 'auto' && VEHICLE_ICONS[iconKey]) {
    return VEHICLE_ICONS[iconKey];
  }
  return isRf ? RF_REMOTE_SVG : TAG_SIG_SVG;
}

const ICO_TRASH =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 6h18M8 6V4h8v2M19 6l-1 14H6L5 6"/></svg>';

const ICO_EDIT =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 20h9M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4 12.5-12.5z"/></svg>';

const RSSI_BAR_HEIGHTS = [10, 14, 18, 22, 26];

const GITHUB_FW_REPO = 'autolinkpromax/AutoDoor-RF2';
const GITHUB_FW_BASE_URL = 'https://raw.githubusercontent.com/' + GITHUB_FW_REPO + '/main/firmware';
const GITHUB_FW_FALLBACK_URL = 'https://raw.githubusercontent.com/' + GITHUB_FW_REPO + '/master/firmware';

