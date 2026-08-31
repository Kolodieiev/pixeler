#include "ChessContext.h"

#include "../../WidgetCreator.h"
#include "ChessClientContext.h"
#include "ChessOfflineContext.h"
#include "ChessPrefContext.h"
#include "ChessServerContext.h"
#include "context/games/GameListContext.h"
#include "widget/menu/FixedMenu.h"
#include "widget/text/TextBox.h"

namespace chess
{
  static const uint8_t MENU_ITEM_NUM{5};

  //----------------------------------------------------------------------------------------------------------

  ChessContext::ChessContext()
  {
    setCpuFrequency(FREQ_MIN);

    showMainTmpl();
  }

  ChessContext::~ChessContext()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  bool ChessContext::loop()
  {
    return true;
  }

  void ChessContext::update()
  {
    (this->*_state_input_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessContext::showMainTmpl()
  {
    _state_input_handler = &ChessContext::procMainMenu;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    FixedMenu* menu = new FixedMenu(ID_MAIN_MENU);
    layout->addWidget(menu);
    menu->setBackColor(COLOR_MAIN_BACK);
    menu->setWidth(UI_WIDTH);
    menu->setHeight(UI_HEIGHT);
    menu->setItemHeight(UI_HEIGHT / MENU_ITEM_NUM - 2);
    menu->setLoopState(true);

    // Один гравець
    MenuItem* solo_item = WidgetCreator::getMenuItem(ID_ITEM_ONE_PLAYER);
    menu->addItem(solo_item);

    Label* solo_lbl = WidgetCreator::getItemLabel(STR_MODE_ONE_PL, font_10x20);
    solo_item->setLabel(solo_lbl);

    // Два гравці
    MenuItem* multi_item = WidgetCreator::getMenuItem(ID_ITEM_TWO_PLAYERS);
    menu->addItem(multi_item);

    Label* multi_lbl = WidgetCreator::getItemLabel(STR_MODE_TWO_PL, font_10x20);
    multi_item->setLabel(multi_lbl);

    // Клієнт
    MenuItem* client_item = WidgetCreator::getMenuItem(ID_ITEM_CLIENT);
    menu->addItem(client_item);

    Label* client_lbl = WidgetCreator::getItemLabel(STR_MODE_CLIENT, font_10x20);
    client_item->setLabel(client_lbl);

    // Сервер
    MenuItem* server_item = WidgetCreator::getMenuItem(ID_ITEM_SERVER);
    menu->addItem(server_item);

    Label* server_lbl = WidgetCreator::getItemLabel(STR_MODE_SERVER, font_10x20);
    server_item->setLabel(server_lbl);

    // Налаштування
    MenuItem* prefs_item = WidgetCreator::getMenuItem(ID_ITEM_PREFS);
    menu->addItem(prefs_item);

    Label* prefs_lbl = WidgetCreator::getItemLabel(STR_PREFERENCES, font_10x20);
    prefs_item->setLabel(prefs_lbl);
  }

  void ChessContext::procMainMenu()
  {
    FixedMenu* menu = getLayout()->getWidgetByID(ID_MAIN_MENU)->castTo<FixedMenu>();

    if (_input.isHolded(BtnID::BTN_UP))
    {
      menu->focusUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      menu->focusDown();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      uint16_t id = menu->getCurrItemID();

      switch (id)
      {
        case ID_ITEM_ONE_PLAYER:
          openContext(new ChessOfflineContext(1));
          break;

        case ID_ITEM_TWO_PLAYERS:
          openContext(new ChessOfflineContext(2));
          break;

        case ID_ITEM_CLIENT:
          openContext(new ChessClientContext());
          break;

        case ID_ITEM_SERVER:
          openContext(new ChessServerContext());
          break;

        case ID_ITEM_PREFS:
          openContext(new ChessPrefContext());
          break;
      }
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      openContext(new GameListContext());
    }
  }
}  // namespace chess
