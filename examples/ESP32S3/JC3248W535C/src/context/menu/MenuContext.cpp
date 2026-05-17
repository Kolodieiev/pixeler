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

uint8_t MenuContext::_last_page_pos;

MenuContext::MenuContext()
{
  //
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  //
  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setTouchSupport(true);
  _menu->setBackColor(COLOR_MAIN_BACK);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - DISPLAY_PADDING * 2);
  _menu->setHeight(UI_HEIGHT - DISPLAY_PADDING * 2);
  _menu->setItemHeight(_menu->getHeight() / MENU_ITEMS_NUM - 2);
  _menu->setPos(DISPLAY_PADDING, DISPLAY_PADDING);
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
  _scrollbar->setMax(_menu->getPagesCount());

  _menu->setPageNum(_last_page_pos);
  _scrollbar->setValue(_last_page_pos);
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
  ITouchscreen::Swipe swipe = _input.getSwipe();

  if (swipe == ITouchscreen::SWIPE_L)
  {
    _last_page_pos = 0;
    openContextByID(ID_CONTEXT_HOME);
  }
  else if (swipe == ITouchscreen::SWIPE_U)
  {
    _input.lock(CLICK_LOCK);
    up();
  }
  else if (swipe == ITouchscreen::SWIPE_D)
  {
    _input.lock(CLICK_LOCK);
    down();
  }
  else if (_input.isReleased())
  {
    _input.lock(CLICK_LOCK);
    ok();
  }
}

void MenuContext::up()
{
  _menu->pageUp();
  _scrollbar->setValue(_menu->getPageNum());
}

void MenuContext::down()
{
  _menu->pageDown();
  _scrollbar->setValue(_menu->getPageNum());
}

void MenuContext::ok()
{
  uint16_t x = _input.getTouchX();
  uint16_t y = _input.getTouchY();
  IWidget* menu_item = _menu->findTouchableAt(x, y);
  if (!menu_item)
    return;

  _last_page_pos = _menu->getPageNum();

  uint16_t item_id = menu_item->getID();

  if (item_id != ID_CONTEXT_MP3 && item_id != ID_CONTEXT_FILES && item_id != ID_CONTEXT_WIFI)
    return;

  openContextByID((ContextID)item_id);
}
