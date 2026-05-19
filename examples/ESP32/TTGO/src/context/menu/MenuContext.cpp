#include "MenuContext.h"
//
#include "../WidgetCreator.h"
#include "./res/headphones.h"
#include "./res/wifi_ico.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/item/MenuItem.h"

#define ICO_WH 35

const char STR_MUSIC_ITEM[] = "Музика";
const char STR_WIFI_ITEM[] = "Підключення";

uint8_t MenuContext::_last_sel_item_pos;

MenuContext::MenuContext()
{
  _display.enableBackLight();
  //
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  //
  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MAIN_BACK);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - DISPLAY_PADDING * 2);
  _menu->setHeight(UI_HEIGHT);
  _menu->setItemHeight(UI_HEIGHT / 4 - 2);
  _menu->setPos(DISPLAY_PADDING, 0);
  // _menu->setLoopState(true);
  //
  _scrollbar = new ScrollBar(ID_SCROLLBAR);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(_menu->getHeight());
  _scrollbar->setPos(_menu->getWidth() + _menu->getXPos(), _menu->getYPos());
  _scrollbar->setBackColor(COLOR_MAIN_BACK);

  // Музика
  MenuItem* mp3_item = WidgetCreator::getMenuItem(ID_CONTEXT_MP3);
  _menu->addItem(mp3_item);

  Image* mp3_img = new Image(1);
  mp3_item->setImg(mp3_img);
  mp3_img->setTransparency(true);
  mp3_img->setWidth(ICO_WH);
  mp3_img->setHeight(ICO_WH);
  mp3_img->setSrc(HEADPHONES_IMG);

  Label* mp3_lbl = WidgetCreator::getItemLabel(STR_MUSIC_ITEM, font_10x20);
  mp3_item->setLbl(mp3_lbl);

  // WiFi
  MenuItem* wifi_item = WidgetCreator::getMenuItem(ID_CONTEXT_WIFI);
  _menu->addItem(wifi_item);

  Image* wifi_img = new Image(1);
  wifi_item->setImg(wifi_img);
  wifi_img->setTransparency(true);
  wifi_img->setWidth(ICO_WH);
  wifi_img->setHeight(ICO_WH);
  wifi_img->setSrc(WIFI_IMG);

  Label* wifi_lbl = WidgetCreator::getItemLabel(STR_WIFI_ITEM, font_10x20);
  wifi_item->setLbl(wifi_lbl);

  //
  _scrollbar->setMax(_menu->getSize());

  _menu->setCurrFocusPos(_last_sel_item_pos);
  _scrollbar->setValue(_last_sel_item_pos);
}

MenuContext::~MenuContext()
{
}

bool MenuContext::loop()
{
  return true;
}

void MenuContext::update()
{
  // if (_input.isHolded(BtnID::BTN_UP))
  // {
  //   _input.lock(BtnID::BTN_UP, HOLD_LOCK);
  //   up();
  // }
  // else if (_input.isHolded(BtnID::BTN_DOWN))
  // {
  //   _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
  //   down();
  // }
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);
    // ok();
    down();
  }
}

void MenuContext::up()
{
  _menu->focusUp();
  _scrollbar->scrollUp();
}

void MenuContext::down()
{
  _menu->focusDown();
  _scrollbar->scrollDown();
}

void MenuContext::ok()
{
  uint16_t id = _menu->getCurrItemID();
  _last_sel_item_pos = _menu->getCurrFocusPos();
  openContextByID((ContextID)id);
}
