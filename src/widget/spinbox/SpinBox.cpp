#pragma GCC optimize("O3")
#include "SpinBox.h"

namespace pixeler
{

  SpinBox::SpinBox(uint16_t widget_ID) : Label(widget_ID, TYPE_SPINBOX)
  {
    setAlign(IWidget::ALIGN_CENTER);
    setGravity(IWidget::GRAVITY_CENTER);
  }

  void SpinBox::copyTo(IWidget* widget) const
  {
    Label::copyTo(widget);

    SpinBox* clone = static_cast<SpinBox*>(widget);
    clone->_min = _min;
    clone->_max = _max;
    clone->_value = _value;
    clone->_step = _step;
    clone->_spin_type = _spin_type;
  }

  SpinBox* SpinBox::clone(uint16_t id) const
  {
    try
    {
      SpinBox* clone = new SpinBox(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void SpinBox::setMin(float min)
  {
    _min = min;

    if (_value < _min)
    {
      _value = _min;
    }

    setSpinValToDraw();
  }

  float SpinBox::getMin() const
  {
    return _min;
  }

  void SpinBox::setMax(float max)
  {
    _max = max;
    if (_value > _max)
    {
      _value = _max;
    }

    setSpinValToDraw();
  }

  float SpinBox::getMax() const
  {
    return _max;
  }

  void SpinBox::setValue(float value)
  {
    if (value < _min)
      _value = _min;
    else if (value > _max)
      _value = _max;
    else
      _value = value;

    setSpinValToDraw();
  }

  float SpinBox::getValue() const
  {
    return _value;
  }

  void SpinBox::setType(SpinType spin_type)
  {
    _spin_type = spin_type;
    setSpinValToDraw();
  }

  SpinBox::SpinType SpinBox::getType() const
  {
    return _spin_type;
  }

  void SpinBox::setStep(float step)
  {
    _step = __builtin_abs(step);
    _is_changed = true;
  }

  float SpinBox::getStep() const
  {
    return _step;
  }

  void SpinBox::setSpinValToDraw()
  {
    if (_spin_type == TYPE_INT)
    {
      int64_t temp = _value;
      setText(String(temp));
    }
    else
    {
      setText(String(_value));
    }
  }

  void SpinBox::up()
  {
    if (_value + _step > _max)
      _value = _min;
    else
      _value += _step;

    setSpinValToDraw();
  }

  void SpinBox::down()
  {
    if (_value - _step < _min)
      _value = _max;
    else
      _value -= _step;

    setSpinValToDraw();
  }
}  // namespace pixeler