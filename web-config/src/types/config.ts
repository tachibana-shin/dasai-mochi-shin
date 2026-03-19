export interface WifiEntry {
  ssid: string;
  pass: string;
}

export interface AlarmEntry {
  enabled: boolean;
  hour: number;
  minute: number;
  repeat: number; // Bitmask: 1=Mon, 2=Tue, 4=Wed, 8=Thu, 16=Fri, 32=Sat, 64=Sun, 128=Once
}

export interface DrinkConfig {
  enabled: boolean;
  startHour: number;
  endHour: number;
  intervalMinutes: number;
  durationSeconds: number;
  dailyGoalLiters: number;
}

export interface AudioConfig {
  alarmEnabled: boolean;
  drinkEnabled: boolean;
  notifyEnabled: boolean;
  volume: number; // 0-21
}

export interface AppConfig {
  brightness: number;
  wifiEnabled: boolean;
  bluetoothEnabled: boolean;
  weatherInterval: number;
  autoOffHour: number;
  autoOnHour: number;

  wifi: WifiEntry[];
  alarms: AlarmEntry[];
  drink: DrinkConfig;
  audio: AudioConfig;

  pinScreenSDA: number;
  pinScreenSCL: number;
  pinSensorTap: number;
  pinSdCS: number;
  pinSdMOSI: number;
  pinSdCLK: number;
  pinSdMISO: number;
  pinAudioLRC: number;
  pinAudioDIN: number;
  pinAudioBCLK: number;

  // Metadata & Localization
  wifiAPName: string;
  bluetoothName: string;
  ntpServer: string;
  gmtOffset_sec: number;
  daylightOffset_sec: number;
  is24Hour: boolean;
  langCode: string;

  // Weather & Paths
  weatherServer: string;
  homePath: string;

  // Mochi Mode settings
  mochiSpeedDivisor: number;
  mochiNegative: boolean;
  mochiClockInterval: number;
  mochiClockDuration: number;

  // Display Hardware settings
  screenFlipMode: boolean;
  screenNegative: boolean;
  screenWidth: number;
  screenHeight: number;

  // Sound paths
  customClickSoundPath: string;
  customDrinkSoundPath: string;
  customAlarmSoundPath: string;
  customNotifySoundPath: string;
}
