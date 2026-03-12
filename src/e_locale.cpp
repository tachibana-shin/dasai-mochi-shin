#include "e_locale.h"

#include "config.h"

const LocaleInfo locale_vi = {
    "SA",
    "CH",
    {"thg 1", "thg 2", "thg 3", "thg 4", "thg 5", "thg 6", "thg 7", "thg 8",
     "thg 9", "thg 10", "thg 11", "thg 12"},
    {"CN", "T2", "T3", "T4", "T5", "T6", "T7"}};

const LocaleInfo locale_en = {
    "AM",
    "PM",
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
     "Nov", "Dec"},
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}};

const LocaleInfo* getActiveLocale() {
  if (config.langCode == "vi") {
    return &locale_vi;
  }
  return &locale_en;
}