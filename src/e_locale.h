#ifndef E_LOCALE_H
#define E_LOCALE_H

enum MsgId {
  MSG_AM,
  MSG_PM,
  MSG_BRIGHTNESS,
  MSG_WIFI,
  MSG_BLUETOOTH,
  MSG_LANGUAGE,
  MSG_GMT_OFFSET,
  MSG_FLIP_SCREEN,
  MSG_NEGATIVE,
  MSG_DRINK_REM,
  MSG_ALARM,
  MSG_ALARM_REP,
  MSG_ALARM_HOUR,
  MSG_ALARM_MIN,
  MSG_AUTO_OFF_H,
  MSG_AUTO_ON_H,
  MSG_WEATHER_INT,
  MSG_MOCHI_SPD,
  MSG_MOCHI_NEG,
  MSG_ALARM_SOUND,
  MSG_DRINK_SOUND,
  MSG_NOTIFY_SOUND,
  MSG_VOLUME,
  MSG_BACK,
  MSG_DRINK_NOW,
  MSG_DRINK_DESC,
  MSG_DRINK_CONFIRM,
  MSG_MISSED_DRINK,
  MSG_DRINK_GREAT,
  MSG_ALARM_TITLE,
  MSG_ALARM_STOP,
  MSG_WIFI_FAILED,
  MSG_WIFI_CONNECTED,
  MSG_NO_GIFS,
  MSG_ON,
  MSG_OFF,
  MSG_TIME_FORMAT,
  MSG_MOCHI_CLK_INT,
  MSG_MOCHI_CLK_DUR,
  MSG_DRINK_START,
  MSG_DRINK_END,
  MSG_DRINK_INTERVAL,
  MSG_DRINK_GOAL,
  MSG_CLEAR_MISSED,
  MSG_COUNT
};

struct LocaleInfo {
  const char* am;
  const char* pm;
  const char* months[12];
  const char* days[7];
  const char* messages[MSG_COUNT];
};

extern const LocaleInfo locale_vi;
extern const LocaleInfo locale_en;

const LocaleInfo* getActiveLocale();
const char* L(MsgId id);

#endif
