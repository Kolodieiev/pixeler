#pragma GCC optimize("O3")
#include "Notification.h"

namespace pixeler
{
  Notification::Notification(uint16_t widget_ID) : IWidget(widget_ID, TYPE_NOTIFICATION)
  {
    setBorderColor(COLOR_ORANGE);
    setBorder(true);

    _left_lbl = new Label(1);
    _left_lbl->setParent(this);
    _left_lbl->setAlign(ALIGN_CENTER);
    _left_lbl->setGravity(GRAVITY_CENTER);
    //
    _right_lbl = _left_lbl->clone(1);
    //
    _msg_lbl = _left_lbl->clone(1);
    _msg_lbl->setMultiline(true);
    //
    _title_lbl = _left_lbl->clone(1);
    _title_lbl->setAutoscroll(true);

    _width = 10;
    _height = 10;
  }

  Notification::~Notification()
  {
    delete _title_lbl;
    delete _msg_lbl;
    delete _left_lbl;
    delete _right_lbl;
  }

  void Notification::onDraw()
  {
    clear();

    // Заголовок
    _title_lbl->setPos(1, 1);
    _title_lbl->setWidth(_width - 2);
    // Ліва кнопка
    _left_lbl->setPos(1, _height - _left_lbl->getHeight() - 1);
    _left_lbl->setWidth((_width - 2) / 2);
    // Права кнопка
    _right_lbl->setPos(_left_lbl->getXPosLoc() + _left_lbl->getWidth(), _left_lbl->getYPosLoc());
    _right_lbl->setWidth(_left_lbl->getWidth());
    // Основне повідомлення
    const uint8_t V_PADDING{10};
    _msg_lbl->setPos(V_PADDING, _title_lbl->getHeight() + 2);
    _msg_lbl->setWidth(_width - V_PADDING * 2);
    _msg_lbl->setHeight(_height - _title_lbl->getHeight() - _left_lbl->getHeight() - 6);
    //
    _title_lbl->onDraw();
    _msg_lbl->onDraw();
    _left_lbl->onDraw();
    _right_lbl->onDraw();
  }

  Notification* Notification::clone(uint16_t id) const
  {
    log_e("Клонування цього віджета неможливе");
    esp_restart();
    return nullptr;
  }

  void Notification::setTitleText(const String &str)
  {
    _title_lbl->setText(str);
  }

  void Notification::setMsgText(const String &str)
  {
    _msg_lbl->setText(str);
  }

  void Notification::setLeftText(const String &str)
  {
    _left_lbl->setText(str);
  }

  void Notification::setRightText(const String &str)
  {
    _right_lbl->setText(str);
  }

  Label* Notification::getTitleLbl()
  {
    return _title_lbl;
  }

  Label* Notification::getMsgLbl()
  {
    return _msg_lbl;
  }

  Label* Notification::getLeftLbl()
  {
    return _left_lbl;
  }

  Label* Notification::getRightLbl()
  {
    return _right_lbl;
  }

}  // namespace pixeler
