#include "e_locale.h"

#include "config.h"

const LocaleInfo locale_vi = {
    "SA",
    "CH",
    {"thg 1", "thg 2", "thg 3", "thg 4", "thg 5", "thg 6", "thg 7", "thg 8",
     "thg 9", "thg 10", "thg 11", "thg 12"},
    {"CN", "T2", "T3", "T4", "T5", "T6", "T7"},
    {
        "SA", "CH", "Độ sáng", "WiFi", "Bluetooth", "Ngôn ngữ", "Múi giờ",
        "Xoay màn hình", "Màu âm bản", "Nhắc uống nước", "Báo thức", "Lặp lại",
        "Giờ báo thức", "Phút báo thức", "Giờ tự tắt", "Giờ tự bật",
        "Cập nhật t.tiết", "Tốc độ Mochi", "Mochi âm bản", "Âm báo thức",
        "Âm nhắc uống", "Âm thông báo", "Âm lượng", "Quay lại",
        "Đã tới giờ uống nước!", "Hãy uống 200ml nhé", "Nhấn 1 click để xác nhận",
        "Bỏ lỡ lần uống nước!", "Rất tốt! Đã ghi nhận.", "BÁO THỨC!", "Nhấn 1 click để dừng",
        "WiFi lỗi", "WiFi đã kết nối", "Không tìm thấy file GIF", "Bật", "Tắt"
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
        "Flip Screen", "Negative", "Drink Rem", "Alarm", "Repeat",
        "Alarm Hour", "Alarm Min", "Auto Off H", "Auto On H",
        "Weather Int", "Mochi Spd", "Mochi Neg", "Alarm Sound",
        "Drink Sound", "Notify Sound", "Volume", "Back",
        "It's time to drink water!", "Please drink 200ml", "Single click to confirm",
        "Missed drink reminder!", "Great! Recorded.", "ALARM!", "Single click to stop",
        "WiFi Failed", "WiFi Connected", "No GIF files found", "ON", "OFF"
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