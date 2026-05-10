#pragma GCC optimize("O3")
#include "Image.h"

namespace pixeler
{
  Image::Image(uint16_t widget_ID) : IWidget(widget_ID, TYPE_IMAGE) {}

  void Image::setTransparency(bool state)
  {
    _has_transparency = state;
    _is_changed = true;
  }

  void Image::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    Image* clone = static_cast<Image*>(widget);
    clone->_img_ptr = _img_ptr;
    clone->_has_transparency = _has_transparency;
  }

  Image* Image::clone(uint16_t id) const
  {
    try
    {
      Image* clone = new Image(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void Image::setSrc(const uint16_t* image_ptr)
  {
    _is_changed = true;
    _img_ptr = image_ptr;
  }

  Image::~Image()
  {
  }

  void Image::onDraw()
  {
    if (!_is_changed)
      return;

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      hide();
      return;
    }

    uint16_t x_offset{0};
    uint16_t y_offset{0};

    if (_parent)
    {
      x_offset = _parent->getXPos();
      y_offset = _parent->getYPos();
    }

    if (_img_ptr)
    {
      if (!_has_transparency)
      {
#if CONFIG_IDF_TARGET_ESP32P4
        if (_width * _height > PPA_IMG_SIZE_TRIGG)
        {
          bool old_state = _display.isPPAEnabled();

          _display.setPPAState(true);
          _display.drawBitmap(_x_pos + x_offset, _y_pos + y_offset, _img_ptr, _width, _height);
          _display.setPPAState(old_state);
        }
        else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
        {
          _display.drawBitmap(_x_pos + x_offset, _y_pos + y_offset, _img_ptr, _width, _height);
        }
      }
      else
      {
        _display.drawBitmapTransp(_x_pos + x_offset, _y_pos + y_offset, _img_ptr, _width, _height);
      }
    }
    else
    {
      log_e("Не встановлено src");
      esp_restart();
    }
  }
}  // namespace pixeler