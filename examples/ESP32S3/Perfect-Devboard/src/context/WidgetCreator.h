/*
 * Генератор віджетів.
 * Використовується тут для часткового очищення файлів вікон від однотипного коду.
 */

#pragma once

#include "driver/graphics/DisplayWrapper.h"
#include "widget/image/Image.h"
#include "widget/keyboard/Keyboard.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/DynamicMenu.h"
#include "widget/menu/item/MenuItem.h"
#include "widget/text/Label.h"
#include "resources/ch32_pins_def.h"
#include "resources/colors.h"
#include "resources/kb_btn_id.h"
#include "resources/strings.h"
#include "resources/ui_const.h"

using namespace pixeler;

class WidgetCreator
{
public:
  static EmptyLayout* getEmptyLayout();
  static Label* getItemLabel(const char* text, const uint8_t* font_ptr = font_unifont, uint8_t text_size = 1);
  static MenuItem* getMenuItem(uint16_t id = 1);
  static DynamicMenu* getDynamicMenu(uint16_t id);
  static Label* getStatusMsgLable(uint16_t id, const char* text, uint8_t text_size = 1);
  static Keyboard* getStandardEnKeyboard(uint16_t id);
  static Keyboard* getCapsdEnKeyboard(uint16_t id);
  static Label* getWindowHeader(uint16_t id, const char* text);
};
