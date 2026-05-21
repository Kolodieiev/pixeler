#include "PrefBrightContext.h"

#include "../../WidgetCreator.h"
#include "manager/SettingsManager.h"

#define MAX_BRIGHT 250
#define BRIGHT_STEP 10

PrefBrightContext::PrefBrightContext()
{
  String bright = SettingsManager::get(STR_PREF_BRIGHT);

  if (bright.equals(""))
    _old_bright = 100;
  else
    _old_bright = atoi(bright.c_str());

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  //
  layout->addWidget(WidgetCreator::getWindowHeader(ID_HEADER, STR_BRIGHT));
  //
  _progress = new ProgressBar(ID_PROGRESSBAR);
  layout->addWidget(_progress);
  _progress->setBackColor(COLOR_BLACK);
  _progress->setProgressColor(COLOR_ORANGE);
  _progress->setBorderColor(COLOR_WHITE);
  _progress->setMax(MAX_BRIGHT);
  _progress->setWidth(UI_WIDTH - 5 * 8);
  _progress->setHeight(20);
  _progress->setProgress(_old_bright);
  _progress->setPos((UI_WIDTH - _progress->getWidth()) / 2, 40);
}

bool PrefBrightContext::loop()
{
  return true;
}

void PrefBrightContext::update()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);

    SettingsManager::set(STR_PREF_BRIGHT, String(_progress->getProgress()).c_str());
    release();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);

    _display.setBrightness(_old_bright);
    release();
  }
  else if (_input.isHolded(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, HOLD_LOCK);

    uint16_t cur_progress = _progress->getProgress();

    if (cur_progress < MAX_BRIGHT)
    {
      cur_progress += BRIGHT_STEP;
      _progress->setProgress(cur_progress);
      _display.setBrightness(cur_progress);
    }
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);

    uint16_t cur_progress = _progress->getProgress();
    if (cur_progress > BRIGHT_STEP * 2)
    {
      cur_progress -= BRIGHT_STEP;
      _progress->setProgress(cur_progress);
      _display.setBrightness(cur_progress);
    }
  }
}