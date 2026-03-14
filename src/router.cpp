#include "router.h"

#include <vector>

#include "clock.h"
#include "dasai_mochi.h"
#include "display.h"
#include "menu.h"
#include "weather.h"
#include "wifi_manager.h"

// 初期状態は CLOCK のみ
std::vector<Route> Router::stack = {Route::DASAI_MOCHI};

// 新しい画面を積む（push）
void Router::push(Route r) { stack.push_back(r); }

// 一つ前に戻る（pop）
void Router::pop() {
  if (stack.size() > 1) {
    stack.pop_back();
  }
}

// 現在のルートを置き換える
void Router::replace(Route r) {
  if (!stack.empty()) {
    stack.back() = r;
  }
}

// 履歴をクリアして新しいルートのみ
void Router::clearAndPush(Route r) {
  stack.clear();
  stack.push_back(r);
}

// 現在のルートを取得
Route Router::current() { return stack.back(); }

// 現在の画面の処理を実行
void Router::loop() {
  switch (current()) {
    case Route::CLOCK:
      loopWeather();
      if (!screenOn) break;
      loopClock();
      break;

    case Route::WIFI_MANAGER:
      if (!screenOn) break;
      loopWiFiManager();
      break;

    case Route::DASAI_MOCHI:
      if (!screenOn) break;
      loopDasaiMochi();
      break;

    case Route::SETTINGS:
      if (!screenOn) break;
      loopSettings();
      break;

    default:
      break;
  }
}