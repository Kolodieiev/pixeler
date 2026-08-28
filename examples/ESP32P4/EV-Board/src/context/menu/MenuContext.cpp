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
#include "context/files/FilesContext.h"
#include "context/home/HomeContext.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/item/MenuItem.h"

#define ICO_WH 35
#define MENU_ITEMS_NUM 4

const char STR_MUSIC_ITEM[] = "Музика";
const char STR_READER_ITEM[] = "Читалка";
const char STR_W_TALKIE_ITEM[] = "Рація";
const char STR_FILES_ITEM[] = "Файли";
const char STR_WIFI_ITEM[] = "Підключення";
const char STR_FIRMWARE_ITEM[] = "Прошивка";
const char STR_GAME_ITEM[] = "Ігри";

uint8_t MenuContext::_last_page_pos;

MenuContext::MenuContext()
{
  setCpuFrequency(FREQ_MIN);
  
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
  MenuItem* files_item = WidgetCreator::getMenuItem(ID_ITEM_FILES);
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
  MenuItem* mp3_item = WidgetCreator::getMenuItem(ID_ITEM_MP3);
  _menu->addItem(mp3_item);

  Image* mp3_img = new Image(1);
  mp3_item->setImg(mp3_img);
  mp3_img->setTransparency(true);
  mp3_img->setWidth(ICO_WH);
  mp3_img->setHeight(ICO_WH);
  mp3_img->setSrc(HEADPHONES_IMG);

  Label* mp3_lbl = WidgetCreator::getItemLabel(STR_MUSIC_ITEM, font_10x20);
  mp3_item->setLbl(mp3_lbl);

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
    openContext(new HomeContext());
  }
  else if (swipe == ITouchscreen::SWIPE_U)
  {
    up();
  }
  else if (swipe == ITouchscreen::SWIPE_D)
  {
    down();
  }
  else if (_input.isReleased())
  {
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

  IContext* context{nullptr};

  switch (item_id)
  {
    case ID_ITEM_FILES:
      context = new FilesContext();
      break;
    default:
      log_e("Невідомий ідентифікатор контексту");
      break;
  }

  if (context)
    openContext(context);
}
