/**
 * @file EditorContext.h
 * @brief Повноекранний контекст текстового редактора на базі віджета TextArea
 */
#pragma once
#pragma GCC optimize("O3")

#include "context/IContext.h"
#include "widget/text/TextArea.h"

using namespace pixeler;

class EditorContext : public IContext
{
public:
  EditorContext();
  virtual ~EditorContext() override;

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum WidgetID : uint8_t
  {
    ID_TEXTAREA = 1,
  };

  enum CursorAction : uint8_t
  {
    CUR_LEFT = 0,
    CUR_RIGHT,
    CUR_UP,
    CUR_DOWN,
    CUR_START,
    CUR_END,
  };

  struct Utf8Char
  {
    char bytes[5];
  };

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
  static void keyPressedHandler(const EspUsbHostKeyboardEvent& event, void* arg);
#endif

  /**
   * @brief Переміщує курсор у заданому напрямку з урахуванням стану виділення.
   *
   * @param action Напрямок або цільова позиція курсора.
   * @param select Якщо true — вмикає/утримує режим виділення, інакше скидає виділення.
   */
  void handleCursorMove(CursorAction action, bool select);

  /**
   * @brief Перемикає розкладку клавіатури між EN та UA.
   */
  void toggleKeyboardLayout();

  /**
   * @brief Виділяє весь текст у редакторі.
   */
  void selectAll();

  /**
   * @brief Знімає обробники клавіатури та виконує перехід у MenuContext.
   */
  void exitToMenu();

private:
  TextArea* _editor{nullptr};
  bool _is_ua_layout{false};
};
