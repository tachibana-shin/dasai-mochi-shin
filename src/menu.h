#ifndef MENU_H
#define MENU_H

#include <Arduino.h>

#include <functional>
#include <vector>

enum class MenuItemType { TOGGLE, ACTION, RANGE, SUBMENU, BACK };

struct MenuItem {
  String label;
  MenuItemType type;
  std::function<String()> getValue;
  std::function<void()> onAction;     // Double click
  std::function<void(int)> onAdjust;  // For range
  int minVal = 0;
  int maxVal = 100;
  int step = 1;

  MenuItem(String l, MenuItemType t) : label(l), type(t) {}
};

void initMenu();
void loopSettings();
void handleMenuClick();
void handleMenuDoubleClick();
void handleMenuTripleClick();

#endif
