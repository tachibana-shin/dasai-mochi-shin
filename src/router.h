#ifndef ROUTER_H
#define ROUTER_H

#include <Arduino.h>

#include <vector>

enum class Route : uint8_t {
  CLOCK,
  WIFI_MANAGER,
  DASAI_MOCHI,
  SETTINGS,
};

class Router {
 public:
  // ルート履歴（スタック）
  static std::vector<Route> stack;

  static void push(Route r);          // 新しい画面に移動
  static void pop();                  // 一つ前の画面に戻る
  static void replace(Route r);       // 現在の画面を置き換える
  static void clearAndPush(Route r);  // 履歴を全部削除して遷移
  static Route current();             // 現在の画面を取得
  static void loop();                 // 現在の画面をループ処理
};

#endif