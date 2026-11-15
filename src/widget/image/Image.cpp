#pragma GCC optimize("O3")
#include "Image.h"

namespace pixeler
{
  Image::Image(uint16_t widget_ID) : IWidget(widget_ID, TYPE_IMAGE) {}

  Image::~Image() {}

  void Image::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    Image* clone = static_cast<Image*>(widget);
    clone->_animation = _animation;
    clone->_img_data = _img_data;
    clone->_frames_counter = _frames_counter;
    clone->_anim_pos = _anim_pos;
    clone->_frames_between_anim = _frames_between_anim;
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

  void Image::setSrc(const uint16_t* image_data)
  {
    _img_data = image_data;
    _is_changed = true;
  }

  void Image::setAnimation(const std::vector<const uint16_t*>* animation, uint8_t frames_between)
  {
    if (animation->empty())
    {
      log_e("Вектор анімації не може бути порожнім");
      esp_restart();
    }

    _animation = animation;
    _frames_between_anim = frames_between;
    resetAnimation();
  }

  void Image::resetAnimation()
  {
    _frames_counter = 0;
    _anim_pos = 0;
    _is_changed = true;
  }

  void Image::drawFrame(uint16_t x, uint16_t y, const uint16_t* frame_data)
  {
    if (!_is_transparent)
    {
#if CONFIG_IDF_TARGET_ESP32P4
      if (static_cast<uint32_t>(_width) * _height > PPA_IMG_SIZE_TRIGG)
      {
        const bool old_state = _display.isPPAEnabled();
        _display.setPPA(true);
        _display.drawBitmap(x, y, frame_data, _width, _height);
        _display.setPPA(old_state);
      }
      else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
      {
        _display.drawBitmap(x, y, frame_data, _width, _height);
      }
    }
    else
    {
      _display.drawBitmapTransp(x, y, frame_data, _width, _height);
    }
  }

  void Image::onDraw()
  {
    if (_visibility == INVISIBLE)
    {
      if (_is_changed)
      {
        _is_changed = false;
        hide();
      }
      return;
    }

    if (_animation)
    {
      if (!_is_changed)
      {
        if (_frames_counter < _frames_between_anim)
        {
          ++_frames_counter;
          return;
        }
        _frames_counter = 0;

        if (_anim_pos < _animation->size() - 1)
          ++_anim_pos;
        else
          _anim_pos = 0;
      }
      else
      {
        _is_changed = false;
      }

      // Завжди заливаємо власним фоновим кольором,
      // інакше можуть залишитися незамальовані шматки попереднього кадру
      const bool orig_transp = _is_transparent;
      _is_transparent = false;
      clear();
      _is_transparent = orig_transp;

      drawFrame(getXPos(), getYPos(), (*_animation)[_anim_pos]);
    }
    else if (_img_data)
    {
      if (!_is_changed)
        return;

      _is_changed = false;

      clear();

      drawFrame(getXPos(), getYPos(), _img_data);
    }
    else
    {
      log_e("Не встановлено src або анімацію");
      esp_restart();
    }
  }
}  // namespace pixeler
