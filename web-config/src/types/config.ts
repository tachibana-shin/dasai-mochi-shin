export interface WifiEntry {
  ssid: string;
  pass: string;
}

export interface AudioConfig {
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
  customNotifySoundPath: string;
}
