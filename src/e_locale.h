#ifndef E_LOCALE_H
#define E_LOCALE_H

struct LocaleInfo {
  const char* am;
  const char* pm;
  const char* months[12];
  const char* days[7];
};

extern const LocaleInfo locale_vi;
extern const LocaleInfo locale_en;

const LocaleInfo* getActiveLocale();

#endif
