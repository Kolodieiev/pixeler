#include "AppsContext.h"

#include "../WidgetCreator.h"
#include "batt_test/BattTestContext.h"
#include "context/menu/MenuContext.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/item/MenuItem.h"

static const char STR_BATT_TEST_ITEM[] = "Тест акумуляторів";

using namespace pixeler;

AppsContext::AppsContext()
{
  setCpuFrequency(FREQ_MIN);

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

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

  // Тест акумів
  MenuItem* test_batt_item = WidgetCreator::getMenuItem(ID_ITEM_TEST_BATT);
  _menu->addItem(test_batt_item);

  Label* test_batt_lbl = WidgetCreator::getItemLabel(STR_BATT_TEST_ITEM, font_10x20);
  test_batt_item->setLabel(test_batt_lbl);

  //------------------------

  _scrollbar->setMax(_menu->getSize());
}

AppsContext::~AppsContext()
{
}

bool AppsContext::loop()
{
  return true;
}

void AppsContext::update()
{
  if (_input.isHolded(BtnID::BTN_UP))
    up();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    down();
  else if (_input.isReleased(BtnID::BTN_OK))
    ok();
  else if (_input.isReleased(BtnID::BTN_BACK))
    openContext(new MenuContext());
}

void AppsContext::up()
{
  _menu->focusUp();
  _scrollbar->scrollUp();
}

void AppsContext::down()
{
  _menu->focusDown();
  _scrollbar->scrollDown();
}

void AppsContext::ok()
{
  uint16_t id = _menu->getCurrItemID();

  IContext* context{nullptr};

  switch (id)
  {
    case ID_ITEM_TEST_BATT:
      context = new BattTestContext();
      break;
    default:
      log_e("Невідомий ідентифікатор контексту: %u", id);
      break;
  }

  if (context)
    openContext(context);
}
