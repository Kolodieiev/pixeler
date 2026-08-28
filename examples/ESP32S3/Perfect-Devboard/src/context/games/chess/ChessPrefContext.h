#pragma once

#include "context/IContext.h"
#include "widget/menu/FixedMenu.h"

namespace chess
{
  class ChessPrefContext : public pixeler::IContext
  {
  public:
    ChessPrefContext();
    virtual ~ChessPrefContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
    void showMainTmpl();  // Головне меню
    void procMainMenu();  // Обробка вводу головного меню

    void showDialogNicknameTmpl();                                     // Показати вікно налаштувань нікнейма
    void showDialogServNameTmpl();                                     // Показати вікно налаштувань серверного імені
    void showDialogServPwdTmpl();                                      // Показати вікно налаштувань серверного пароля
    void addDialog(const String& title_txt, const String& start_txt);  // Додати до layout діалог вводу
    void procDialog();                                                 // Обробка вводу діалогового вікна

    void saveDialogResult(String& result_str);

  private:
    using StateHandler = void (ChessPrefContext::*)();

    enum WidgetID : uint8_t
    {
      ID_MAIN_MENU = 1,
      ID_DIALOG_LBL,
      ID_DIALOG_TEXT,
      ID_DIALOG_KB,
    };

    enum ItemID : uint8_t
    {
      ID_ITEM_NICK = 1,
      ID_ITEM_SERV_NAME,
      ID_ITEM_SERV_PWD
    };

    enum PrefDialogID : uint8_t
    {
      DIALOG_ID_NICK = 0,
      DIALOG_ID_SERV_NAME,
      DIALOG_ID_SERV_PWD,
      DIALOG_ID_SERVER_CONN,
    };

    StateHandler _state_kb_handler{nullptr};

    PrefDialogID _dialog_id{DIALOG_ID_NICK};
  };
}  // namespace chess
