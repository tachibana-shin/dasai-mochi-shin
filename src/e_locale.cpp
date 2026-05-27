#include "e_locale.h"

#include "config.h"

const LocaleInfo locale_vi = {
    "SA",
    "CH",
    {"thg1", "thg2", "thg3", "thg4", "thg5", "thg6", "thg7", "thg8",
     "thg9", "thg10", "thg11", "thg12"},
    {"CN", "T2", "T3", "T4", "T5", "T6", "T7"},
    {
        "SA", "CH", "Độ sáng", "WiFi", "Bluetooth", "Ngôn ngữ", "Múi giờ",
        "Xoay màn hình", "Màu âm bản", "Giờ tự tắt", "Giờ tự bật",
        "Cập nhật t.tiết", "Tốc độ Mochi", "Mochi âm bản", "Âm thông báo",
        "Âm lượng", "Quay lại",
        "WiFi lỗi", "WiFi đã kết nối", "Không tìm thấy file GIF", "Bật", "Tắt",
        "Định dạng giờ", "Khoảng cách hiện ĐH", "Thời gian hiện ĐH"
    }
};

const LocaleInfo locale_en = {
    "AM",
    "PM",
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
     "Nov", "Dec"},
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    {
        "AM", "PM", "Brightness", "WiFi", "Bluetooth", "Language", "GMT Offset",
        "Flip Screen", "Negative", "Auto Off H", "Auto On H",
        "Weather Int", "Mochi Spd", "Mochi Neg", "Notify Sound",
        "Volume", "Back",
        "WiFi Failed", "WiFi Connected", "No GIF files found", "ON", "OFF",
        "Time Format", "Mochi Clock Int", "Mochi Clock Dur"
    }
};

const LocaleInfo* getActiveLocale() {
  if (config.langCode == "vi") {
    return &locale_vi;
  }
  return &locale_en;
}

const char* L(MsgId id) {
  if (id >= 0 && id < MSG_COUNT) {
    return getActiveLocale()->messages[id];
  }
  return "";
}