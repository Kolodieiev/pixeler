#include "MenuContext.h"
//
#include "../WidgetCreator.h"
#include "./res/book.h"
#include "./res/chip.h"
#include "./res/headphones.h"
#include "./res/joystick.h"
#include "./res/sd.h"
#include "./res/settings.h"
#include "./res/wifi_ico.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/item/MenuItem.h"

#define ICO_WH 35
#define MENU_ITEMS_NUM 4

const char STR_MUSIC_ITEM[] = "Музика";
const char STR_READER_ITEM[] = "Читалка";
const char STR_W_TALKIE_ITEM[] = "Рація";
const char STR_FILES_ITEM[] = "Файли";
const char STR_GAME_ITEM[] = "Ігри";
const char STR_WIFI_ITEM[] = "Підключення";
const char STR_FIRMWARE_ITEM[] = "Прошивка";

uint8_t MenuContext::_last_sel_item_pos;

MenuContext::MenuContext()
{
  //
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  //
  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MAIN_BACK);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - DISPLAY_PADDING * 2);
  _menu->setHeight(UI_HEIGHT - DISPLAY_CUTOUT * 2 - DISPLAY_PADDING * 2);
  _menu->setItemHeight(_menu->getHeight() / MENU_ITEMS_NUM - 2);
  _menu->setPos(DISPLAY_PADDING, DISPLAY_CUTOUT + DISPLAY_PADDING);
  //
  _scrollbar = new ScrollBar(ID_SCROLLBAR);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(_menu->getHeight());
  _scrollbar->setPos(_menu->getWidth() + _menu->getXPos(), _menu->getYPos());
  _scrollbar->setBackColor(COLOR_MAIN_BACK);

  // Файли
  MenuItem* files_item = WidgetCreator::getMenuItem(ID_CONTEXT_FILES);
  _menu->addItem(files_item);

  Image* files_img = new Image(1);
  files_item->setImg(files_img);
  files_img->setTransparency(true);
  files_img->setWidth(ICO_WH);
  files_img->setHeight(ICO_WH);
  files_img->setSrc(SD_IMG);

  Label* files_lbl = WidgetCreator::getItemLabel(STR_FILES_ITEM, font_10x20);
  files_item->setLbl(files_lbl);

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

  // Ігри
  MenuItem* game_item = WidgetCreator::getMenuItem(ID_CONTEXT_GAMES);
  _menu->addItem(game_item);

  Image* game_img = new Image(1);
  game_item->setImg(game_img);
  game_img->setTransparency(true);
  game_img->setWidth(ICO_WH);
  game_img->setHeight(ICO_WH);
  game_img->setSrc(JOYSTICK_IMG);

  Label* game_lbl = WidgetCreator::getItemLabel(STR_GAME_ITEM, font_10x20);
  game_item->setLbl(game_lbl);

  // Читалка
  MenuItem* read_item = WidgetCreator::getMenuItem(ID_CONTEXT_READER);
  _menu->addItem(read_item);

  Image* read_img = new Image(1);
  read_item->setImg(read_img);
  read_img->setTransparency(true);
  read_img->setWidth(ICO_WH);
  read_img->setHeight(ICO_WH);
  read_img->setSrc(BOOK_IMG);

  Label* read_lbl = WidgetCreator::getItemLabel(STR_READER_ITEM, font_10x20);
  read_item->setLbl(read_lbl);

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

  // Налаштування
  MenuItem* pref_item = WidgetCreator::getMenuItem(ID_CONTEXT_PREF_SEL);
  _menu->addItem(pref_item);

  Image* pref_img = new Image(1);
  pref_item->setImg(pref_img);
  pref_img->setTransparency(true);
  pref_img->setWidth(ICO_WH);
  pref_img->setHeight(ICO_WH);
  pref_img->setSrc(SETTINGS_IMG);

  Label* pref_lbl = WidgetCreator::getItemLabel(STR_PREFERENCES, font_10x20);
  pref_item->setLbl(pref_lbl);

  // Прошивка
  MenuItem* firm_item = WidgetCreator::getMenuItem(ID_CONTEXT_FIRMWARE);
  _menu->addItem(firm_item);

  Image* firm_img = new Image(1);
  firm_item->setImg(firm_img);
  firm_img->setTransparency(true);
  firm_img->setWidth(ICO_WH);
  firm_img->setHeight(ICO_WH);
  firm_img->setSrc(CHIP_IMG);

  Label* firm_lbl = WidgetCreator::getItemLabel(STR_FIRMWARE_ITEM, font_10x20);
  firm_item->setLbl(firm_lbl);
  //
  _scrollbar->setMax(_menu->getSize());

  _menu->setCurrFocusPos(_last_sel_item_pos);
  _scrollbar->setValue(_last_sel_item_pos);

  _input.onKeyPressed(keyPressedHandler, this);
}

MenuContext::~MenuContext()
{
  _input.onKeyPressed(nullptr, nullptr);
}

bool MenuContext::loop()
{
  return true;
}

void MenuContext::update()
{
  if (_input.isHolded(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, HOLD_LOCK);
    up();
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
    down();
  }
  else if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);
    ok();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
    _last_sel_item_pos = 0;
    openContextByID(ID_CONTEXT_HOME);
  }
}

void MenuContext::keyPressedHandler(const EspUsbHostKeyboardEvent& event, void* arg)
{
  MenuContext* self = static_cast<MenuContext*>(arg);

  switch (event.keycode)
  {
    case Input::KEY_UP_ARROW:
      self->post([self]()
                 { self->up(); });
      return;

    case Input::KEY_DOWN_ARROW:
      self->post([self]()
                 { self->down(); });
      return;

    case Input::KEY_ESCAPE:
      self->post([self]()
                 { 
                  self->_last_sel_item_pos = 0;
                  self->openContextByID(ID_CONTEXT_HOME); });
      return;
    case Input::KEY_ENTER:
      self->post([self]()
                 { self->ok(); });
      return;
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
