#include "GameListContext.h"

#include "../WidgetCreator.h"
#include "./icons/chess_ico.h"
#include "./icons/sokoban_ico.h"
#include "context/games/chess/ChessContext.h"
#include "context/games/sokoban/SokobanContext.h"
#include "context/menu/MenuContext.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/item/MenuItem.h"

static const char STR_SOKOBAN_ITEM[] = "Комірник";
static const char STR_CHESS_ITEM[] = "Шахи";

GameListContext::GameListContext()
{
  setCpuFrequency(FREQ_MIN);
  
  //------ Налаштування зовнішнього вигляду віджетів
  //
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  //
  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MENU_ITEM);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - 2);
  _menu->setHeight(UI_HEIGHT - 2);
  _menu->setItemHeight((_menu->getHeight() - 2) / 4);
  //
  _scrollbar = new ScrollBar(ID_SCROLLBAR);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(UI_HEIGHT);
  _scrollbar->setPos(UI_WIDTH - SCROLLBAR_WIDTH, 0);

  //

  MenuItem* sokoban_item = WidgetCreator::getMenuItem(ID_ITEM_SOKOBAN);
  _menu->addItem(sokoban_item);

  Image* soko_img = new Image(1);
  sokoban_item->setImg(soko_img);
  soko_img->setWidth(32);
  soko_img->setHeight(32);
  soko_img->setCornerRadius(5);
  soko_img->setTransparency(true);
  soko_img->setSrc(ICO_SOKOBAN);

  Label* soko_lbl = WidgetCreator::getItemLabel(STR_SOKOBAN_ITEM, font_10x20);
  sokoban_item->setLbl(soko_lbl);
  soko_lbl->setAutoscrollInFocus(true);

  //

  MenuItem* chess_item = WidgetCreator::getMenuItem(ID_ITEM_CHESS);
  _menu->addItem(chess_item);

  Image* chess_img = new Image(1);
  chess_item->setImg(chess_img);
  chess_img->setWidth(32);
  chess_img->setHeight(32);
  chess_img->setCornerRadius(5);
  chess_img->setTransparency(true);
  chess_img->setSrc(ICO_CHESS);

  Label* chess_lbl = WidgetCreator::getItemLabel(STR_CHESS_ITEM, font_10x20);
  chess_item->setLbl(chess_lbl);
  chess_lbl->setAutoscrollInFocus(true);

  _scrollbar->setMax(_menu->getSize());
}

GameListContext::~GameListContext()
{
}

bool GameListContext::loop()
{
  return true;
}

void GameListContext::update()
{
  if (_input.isHolded(BtnID::BTN_UP))
  {
    up();
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    down();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    openContext(new MenuContext());
  }
  else if (_input.isReleased(BtnID::BTN_OK))
  {
    uint16_t id = _menu->getCurrItemID();
    IContext* context{nullptr};
    switch (id)
    {
      case ID_ITEM_SOKOBAN:
        context = new sokoban::SokobanContext();
        break;
      case ID_ITEM_CHESS:
        context = new chess::ChessContext();
        break;
      default:
        log_e("Невідомий ідентифікатор контексту");
        break;
    }

    if (context)
      openContext(context);
  }
}

void GameListContext::up()
{
  _menu->focusUp();
  _scrollbar->scrollUp();
}

void GameListContext::down()
{
  _menu->focusDown();
  _scrollbar->scrollDown();
}
