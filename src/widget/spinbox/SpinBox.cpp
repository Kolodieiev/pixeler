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
    clone->_min_val = _min_val;
    clone->_max_val = _max_val;
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
    _min_val = min;

    if (_value < _min_val)
      _value = _min_val;

    setSpinValToDraw();
  }

  float SpinBox::getMin() const
  {
    return _min_val;
  }

  void SpinBox::setMax(float max)
  {
    _max_val = max;

    if (_value > _max_val)
      _value = _max_val;

    setSpinValToDraw();
  }

  float SpinBox::getMax() const
  {
    return _max_val;
  }

  void SpinBox::setValue(float value)
  {
    if (value < _min_val)
      _value = _min_val;
    else if (value > _max_val)
      _value = _max_val;
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
    _step = std::abs(step);
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
    log_i("_value: %f", _value);
    log_i("_step: %f", _step);
    log_i("_max_val: %f", _max_val);

    if (_value + _step > _max_val)
      _value = _min_val;
    else
      _value += _step;

    setSpinValToDraw();
  }

  void SpinBox::down()
  {
    log_i("_value: %f", _value);
    log_i("_step: %f", _step);
    log_i("_min_val: %f", _min_val);

    if (_value - _step < _min_val)
      _value = _max_val;
    else
      _value -= _step;

    setSpinValToDraw();
  }
}  // namespace pixeler
