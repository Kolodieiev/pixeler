#include "Chart.h"

#include <cmath>
#include <cstdio>

static constexpr uint16_t MIN_PLOT_SIZE{8u};
static constexpr uint8_t TICK_LEN{3u};
static constexpr uint8_t LABEL_GAP{3u};
static constexpr uint8_t ELEMENT_GAP{3u};
static constexpr uint8_t LEGEND_SWATCH_SIZE{6u};
static constexpr uint8_t LEGEND_ITEM_GAP{8u};
static constexpr uint8_t DASH_LEN{3u};
static constexpr uint8_t DASH_STEP{6u};
static constexpr uint8_t MAX_LINE_THICKNESS{3u};
static constexpr uint8_t MAX_MARKER_RADIUS{6u};
static constexpr uint8_t MAX_DECIMALS{4u};
static constexpr uint8_t MAX_TEXT_SIZE{7u};
static constexpr uint8_t MAX_SCALE_ITERS{4u};
static constexpr size_t FMT_BUF_SIZE{32u};
static constexpr float MAX_VIS_STEPS_F{65535.0f};
static constexpr float EPSILON_F{1e-6f};
static constexpr float NICE_TOLERANCE_F{1e-5f};
static constexpr float NICE_BUMP_FACTOR{1.05f};
static constexpr float AUTO_FLAT_PAD_RATIO{0.1f};
static constexpr float MAX_COORD_F{1e6f};
static constexpr float SPAN_INV_GUARD_F{10000.0f};
static constexpr float SCI_FMT_THRESHOLD_F{1e7f};

namespace pixeler
{
  static float calcNiceStep(const float raw_step)
  {
    if (!std::isfinite(raw_step) || raw_step <= EPSILON_F)
      return 1.0f;

    const float exponent = floorf(log10f(raw_step));
    const float magnitude = powf(10.0f, exponent);
    if (!std::isfinite(magnitude) || magnitude <= 0.0f)
      return 1.0f;

    const float fraction = raw_step / magnitude;

    float nice_fraction = 10.0f;
    if (fraction <= 1.0f + NICE_TOLERANCE_F)
      nice_fraction = 1.0f;
    else if (fraction <= 2.0f + NICE_TOLERANCE_F)
      nice_fraction = 2.0f;
    else if (fraction <= 2.5f + NICE_TOLERANCE_F)
      nice_fraction = 2.5f;
    else if (fraction <= 5.0f + NICE_TOLERANCE_F)
      nice_fraction = 5.0f;

    return nice_fraction * magnitude;
  }

  Chart::Chart(const uint16_t widget_ID, const TypeID type_ID) : IWidget(widget_ID, type_ID)
  {
  }

  void Chart::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    Chart* clone = static_cast<Chart*>(widget);
    clone->_series = _series;
    clone->_x_labels = _x_labels;
    clone->_title = _title;
    clone->_x_axis_name = _x_axis_name;
    clone->_y_axis_name = _y_axis_name;
    clone->_x_unit = _x_unit;
    clone->_y_unit = _y_unit;
    clone->_font = _font;
    clone->_title_font = _title_font;
    clone->_x_min = _x_min;
    clone->_x_max = _x_max;
    clone->_x_step = _x_step;
    clone->_y_min = _y_min;
    clone->_y_max = _y_max;
    clone->_eff_y_min = _eff_y_min;
    clone->_eff_y_max = _eff_y_max;
    clone->_plot_back_color = _plot_back_color;
    clone->_axis_color = _axis_color;
    clone->_grid_color = _grid_color;
    clone->_zero_line_color = _zero_line_color;
    clone->_title_color = _title_color;
    clone->_text_color = _text_color;
    clone->_legend_text_color = _legend_text_color;
    clone->_point_step_px = _point_step_px;
    clone->_cached_width = 0u;
    clone->_cached_height = 0u;
    clone->_last_max_points = 0u;
    clone->_plot_w = 0u;
    clone->_plot_h = 0u;
    clone->_chart_type = _chart_type;
    clone->_x_scale_mode = _x_scale_mode;
    clone->_text_size = _text_size;
    clone->_title_size = _title_size;
    clone->_padding = _padding;
    clone->_grid_x_divs = _grid_x_divs;
    clone->_grid_y_divs = _grid_y_divs;
    clone->_line_thickness = _line_thickness;
    clone->_marker_radius = _marker_radius;
    clone->_bar_gap = _bar_gap;
    clone->_x_decimals = _x_decimals;
    clone->_y_decimals = _y_decimals;
    clone->_auto_scale_y = _auto_scale_y;
    clone->_show_grid = _show_grid;
    clone->_grid_dashed = _grid_dashed;
    clone->_show_axes = _show_axes;
    clone->_show_x_labels = _show_x_labels;
    clone->_show_y_labels = _show_y_labels;
    clone->_show_legend = _show_legend;
    clone->_show_markers = _show_markers;
    clone->_show_zero_line = _show_zero_line;
    clone->_is_changed = true;
    clone->_is_data_changed = true;
    clone->_is_layout_dirty = true;
    clone->_has_drawn_once = false;
    clone->_cached_has_border = false;
  }

  Chart* Chart::clone(const uint16_t id) const
  {
    try
    {
      Chart* clone = new Chart(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void Chart::setChartType(const ChartType type)
  {
    if (_chart_type == type)
      return;

    _chart_type = type;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  Chart::ChartType Chart::getChartType() const
  {
    return _chart_type;
  }

  void Chart::setXScaleMode(const XScaleMode mode)
  {
    if (_x_scale_mode == mode)
      return;

    _x_scale_mode = mode;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  Chart::XScaleMode Chart::getXScaleMode() const
  {
    return _x_scale_mode;
  }

  void Chart::setPointStepPx(const uint16_t step_px)
  {
    if (step_px == 0u)
    {
      setXScaleMode(X_SCALE_AUTO);
      return;
    }

    if (_point_step_px == step_px && _x_scale_mode == X_SCALE_FIXED_STEP)
      return;

    _point_step_px = step_px;
    _x_scale_mode = X_SCALE_FIXED_STEP;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  uint16_t Chart::getPointStepPx() const
  {
    return _point_step_px;
  }

  void Chart::setXStep(const float step)
  {
    if (!std::isfinite(step) || step < 0.0f)
      return;

    if (step > 0.0f && step <= EPSILON_F)
      return;

    if (step == _x_step)
      return;

    _x_step = step;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  int16_t Chart::addSeries(const String& name, const uint16_t color, const uint16_t max_points)
  {
    if (max_points == 0u)
      return -1;

    try
    {
      Series series;
      series.data.resize(max_points, 0.0f);
      series.name = name;
      series.color = color;
      series.capacity = max_points;
      _series.push_back(std::move(series));
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      return -1;
    }

    _is_changed = true;
    _is_layout_dirty = true;
    return static_cast<int16_t>(_series.size() - 1u);
  }

  void Chart::removeSeries(const uint16_t series_idx)
  {
    if (series_idx >= _series.size())
      return;

    _series.erase(_series.begin() + series_idx);
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::clearSeriesData(const uint16_t series_idx)
  {
    if (series_idx >= _series.size())
      return;

    Series& series = _series[series_idx];
    series.head = 0u;
    series.count = 0u;
    if (series.visible)
      _is_data_changed = true;
  }

  void Chart::clearAllSeries()
  {
    _series.clear();
    _is_changed = true;
    _is_layout_dirty = true;
  }

  uint16_t Chart::getSeriesCount() const
  {
    return static_cast<uint16_t>(_series.size());
  }

  void Chart::pushPoint(const uint16_t series_idx, const float value)
  {
    if (series_idx >= _series.size())
      return;

    Series& series = _series[series_idx];
    if (series.capacity == 0u)
      return;

    series.data[series.head] = value;
    ++series.head;
    if (series.head >= series.capacity)
      series.head = 0u;

    if (series.count < series.capacity)
      ++series.count;

    if (series.visible)
      _is_data_changed = true;
  }

  void Chart::setSeriesData(const uint16_t series_idx, const float* values, const uint16_t count)
  {
    if (series_idx >= _series.size())
      return;

    Series& series = _series[series_idx];
    if (count == 0u)
    {
      series.head = 0u;
      series.count = 0u;
      if (series.visible)
        _is_data_changed = true;
      return;
    }

    if (!values)
      return;

    bool capacity_changed{false};
    if (count > series.capacity)
    {
      try
      {
        series.data.resize(count);
        series.capacity = count;
        capacity_changed = true;
      }
      catch (const std::bad_alloc& e)
      {
        log_e("%s", e.what());
        return;
      }
    }

    for (uint16_t i{0u}; i < count; ++i)
      series.data[i] = values[i];

    series.head = (count == series.capacity) ? 0u : count;
    series.count = count;

    if (series.visible)
    {
      _is_data_changed = true;
      if (capacity_changed)
        _is_changed = true;
    }
  }

  void Chart::setSeriesColor(const uint16_t series_idx, const uint16_t color)
  {
    if (series_idx >= _series.size())
      return;

    _series[series_idx].color = color;
    _is_changed = true;
  }

  void Chart::setSeriesName(const uint16_t series_idx, const String& name)
  {
    if (series_idx >= _series.size())
      return;

    _series[series_idx].name = name;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setSeriesVisible(const uint16_t series_idx, const bool visible)
  {
    if (series_idx >= _series.size() || _series[series_idx].visible == visible)
      return;

    _series[series_idx].visible = visible;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setTitle(const String& title)
  {
    _title = title;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setXAxisName(const String& name)
  {
    _x_axis_name = name;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setYAxisName(const String& name)
  {
    _y_axis_name = name;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setXUnit(const String& unit)
  {
    _x_unit = unit;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setYUnit(const String& unit)
  {
    _y_unit = unit;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setXLabels(const std::vector<String>& labels)
  {
    _x_labels = labels;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::clearXLabels()
  {
    _x_labels.clear();
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setYRange(const float min_val, const float max_val)
  {
    if (!std::isfinite(min_val) || !std::isfinite(max_val) || max_val <= min_val)
      return;

    const float span = max_val - min_val;
    if (!std::isfinite(span) || !std::isfinite(SPAN_INV_GUARD_F / span))
      return;

    _y_min = min_val;
    _y_max = max_val;
    _eff_y_min = min_val;
    _eff_y_max = max_val;
    _auto_scale_y = false;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setXRange(const float min_val, const float max_val)
  {
    if (!std::isfinite(min_val) || !std::isfinite(max_val))
      return;

    if (max_val > min_val && !std::isfinite(max_val - min_val))
      return;

    _x_min = min_val;
    _x_max = max_val;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setAutoScaleY(const bool state)
  {
    if (_auto_scale_y == state)
      return;

    _auto_scale_y = state;
    if (!state)
    {
      _eff_y_min = _y_min;
      _eff_y_max = _y_max;
    }

    _is_changed = true;
    _is_layout_dirty = true;
  }

  bool Chart::isAutoScaleY() const
  {
    return _auto_scale_y;
  }

  void Chart::setAxisDecimals(const uint8_t x_decimals, const uint8_t y_decimals)
  {
    _x_decimals = (x_decimals > MAX_DECIMALS) ? MAX_DECIMALS : x_decimals;
    _y_decimals = (y_decimals > MAX_DECIMALS) ? MAX_DECIMALS : y_decimals;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setFont(const uint8_t* font)
  {
    _font = font ? font : font_unifont;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setTextSize(const uint8_t size)
  {
    if (size == 0u)
      _text_size = 1u;
    else if (size > MAX_TEXT_SIZE)
      _text_size = MAX_TEXT_SIZE;
    else
      _text_size = size;

    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setTitleFont(const uint8_t* font)
  {
    _title_font = font ? font : font_unifont;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setTitleSize(const uint8_t size)
  {
    if (size == 0u)
      _title_size = 1u;
    else if (size > MAX_TEXT_SIZE)
      _title_size = MAX_TEXT_SIZE;
    else
      _title_size = size;

    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setPlotBackColor(const uint16_t color)
  {
    _plot_back_color = color;
    _is_changed = true;
  }

  void Chart::setAxisColor(const uint16_t color)
  {
    _axis_color = color;
    _is_changed = true;
  }

  void Chart::setGridColor(const uint16_t color)
  {
    _grid_color = color;
    _is_changed = true;
  }

  void Chart::setZeroLineColor(const uint16_t color)
  {
    _zero_line_color = color;
    _is_changed = true;
  }

  void Chart::setTitleColor(const uint16_t color)
  {
    _title_color = color;
    _is_changed = true;
  }

  void Chart::setTextColor(const uint16_t color)
  {
    _text_color = color;
    _is_changed = true;
  }

  void Chart::setLegendTextColor(const uint16_t color)
  {
    _legend_text_color = color;
    _is_changed = true;
  }

  void Chart::setShowGrid(const bool state)
  {
    _show_grid = state;
    _is_changed = true;
  }

  void Chart::setGridDashed(const bool state)
  {
    _grid_dashed = state;
    _is_changed = true;
  }

  void Chart::setGridDivisions(const uint8_t x_divs, const uint8_t y_divs)
  {
    _grid_x_divs = (x_divs == 0u) ? 1u : x_divs;
    _grid_y_divs = (y_divs == 0u) ? 1u : y_divs;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setShowAxes(const bool state)
  {
    _show_axes = state;
    _is_changed = true;
  }

  void Chart::setShowXLabels(const bool state)
  {
    _show_x_labels = state;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setShowYLabels(const bool state)
  {
    _show_y_labels = state;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setShowLegend(const bool state)
  {
    _show_legend = state;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  void Chart::setShowMarkers(const bool state)
  {
    _show_markers = state;
    _is_data_changed = true;
  }

  void Chart::setMarkerRadius(const uint8_t radius)
  {
    if (radius == 0u)
      _marker_radius = 1u;
    else if (radius > MAX_MARKER_RADIUS)
      _marker_radius = MAX_MARKER_RADIUS;
    else
      _marker_radius = radius;

    _is_data_changed = true;
  }

  void Chart::setLineThickness(const uint8_t thickness)
  {
    if (thickness == 0u)
      _line_thickness = 1u;
    else if (thickness > MAX_LINE_THICKNESS)
      _line_thickness = MAX_LINE_THICKNESS;
    else
      _line_thickness = thickness;

    _is_data_changed = true;
  }

  void Chart::setShowZeroLine(const bool state)
  {
    _show_zero_line = state;
    _is_data_changed = true;
  }

  void Chart::setBarGap(const uint8_t gap)
  {
    _bar_gap = gap;
    _is_data_changed = true;
  }

  void Chart::setPadding(const uint8_t padding)
  {
    _padding = padding;
    _is_changed = true;
    _is_layout_dirty = true;
  }

  Chart::XScaleMode Chart::getEffectiveXScaleMode() const
  {
    if (_x_scale_mode == X_SCALE_AUTO)
      return (_chart_type == TYPE_BAR) ? X_SCALE_FIT : X_SCALE_CAPACITY;

    return _x_scale_mode;
  }

  bool Chart::isPhysicalXStepActive() const
  {
    return _x_scale_mode == X_SCALE_AUTO && _x_step > 0.0f && _x_max > _x_min;
  }

  float Chart::getSeriesValue(const Series& series, const uint16_t logical_idx) const
  {
    if (series.count < series.capacity)
      return series.data[logical_idx];

    uint32_t ring_idx = static_cast<uint32_t>(series.head) + logical_idx;
    if (ring_idx >= series.capacity)
      ring_idx -= series.capacity;

    return series.data[ring_idx];
  }

  uint16_t Chart::calcMaxPoints() const
  {
    uint16_t max_pts{0u};
    for (const Series& s : _series)
    {
      if (s.visible && s.count > max_pts)
        max_pts = s.count;
    }

    return max_pts;
  }

  uint16_t Chart::calcMaxCapacity() const
  {
    uint16_t max_cap{0u};
    for (const Series& s : _series)
    {
      if (s.visible && s.capacity > max_cap)
        max_cap = s.capacity;
    }

    return max_cap;
  }

  uint16_t Chart::calcVisibleStartIdx(const uint16_t point_count, const int32_t pw) const
  {
    if (isPhysicalXStepActive())
    {
      const float steps_f = (_x_max - _x_min) / _x_step;
      if (!std::isfinite(steps_f) || steps_f >= MAX_VIS_STEPS_F)
        return 0u;

      const float tol = fmaxf(EPSILON_F, steps_f * NICE_TOLERANCE_F);
      uint32_t max_vis{1u};
      if (_chart_type == TYPE_BAR)
      {
        const float bars_f = floorf(steps_f + tol);
        max_vis = (bars_f >= 1.0f) ? static_cast<uint32_t>(bars_f) : 1u;
      }
      else
      {
        max_vis = static_cast<uint32_t>(floorf(steps_f + tol)) + 1u;
      }

      return (point_count > max_vis) ? static_cast<uint16_t>(point_count - max_vis) : 0u;
    }

    if (getEffectiveXScaleMode() != X_SCALE_FIXED_STEP || pw < static_cast<int32_t>(MIN_PLOT_SIZE))
      return 0u;

    uint32_t max_vis{1u};
    if (_chart_type == TYPE_BAR)
      max_vis = (pw >= static_cast<int32_t>(_point_step_px)) ? (static_cast<uint32_t>(pw) / _point_step_px) : 1u;
    else
      max_vis = static_cast<uint32_t>(pw - 1) / _point_step_px + 1u;

    return (point_count > max_vis) ? static_cast<uint16_t>(point_count - max_vis) : 0u;
  }

  bool Chart::updateAutoScaleRange()
  {
    float data_min{0.0f};
    float data_max{0.0f};
    bool has_valid{false};

    const uint16_t shared_bar_start = (_chart_type == TYPE_BAR) ? calcVisibleStartIdx(calcMaxPoints(), _plot_w) : 0u;

    for (const Series& s : _series)
    {
      if (!s.visible)
        continue;

      const uint16_t start_idx = (_chart_type == TYPE_BAR) ? shared_bar_start : calcVisibleStartIdx(s.count, _plot_w);
      for (uint16_t i{start_idx}; i < s.count; ++i)
      {
        const float v = getSeriesValue(s, i);
        if (!std::isfinite(v))
          continue;

        if (!has_valid)
        {
          data_min = v;
          data_max = v;
          has_valid = true;
        }
        else
        {
          if (v < data_min)
            data_min = v;
          if (v > data_max)
            data_max = v;
        }
      }
    }

    if (!has_valid)
      return false;

    if (_chart_type == TYPE_BAR)
    {
      if (data_min > 0.0f)
        data_min = 0.0f;
      if (data_max < 0.0f)
        data_max = 0.0f;
    }

    const float raw_span = data_max - data_min;
    if (!std::isfinite(raw_span))
      return false;

    if (raw_span < EPSILON_F)
    {
      const float pad = (fabsf(data_min) < EPSILON_F) ? 1.0f : fabsf(data_min) * AUTO_FLAT_PAD_RATIO;
      data_min -= pad;
      data_max += pad;
      if (!std::isfinite(data_max - data_min))
        return false;
    }

    const float divs_f = static_cast<float>(_grid_y_divs);
    float nice_step = calcNiceStep((data_max - data_min) / divs_f);
    float new_min{0.0f};
    float new_max{0.0f};

    if (_chart_type == TYPE_BAR && data_max == 0.0f && data_min < 0.0f)
    {
      new_max = 0.0f;
      new_min = -nice_step * divs_f;
      for (uint8_t iter{0u}; iter < MAX_SCALE_ITERS && new_min - EPSILON_F * nice_step > data_min; ++iter)
      {
        nice_step = calcNiceStep(nice_step * NICE_BUMP_FACTOR);
        new_min = -nice_step * divs_f;
      }
    }
    else
    {
      new_min = floorf(data_min / nice_step) * nice_step;
      new_max = new_min + nice_step * divs_f;
      for (uint8_t iter{0u}; iter < MAX_SCALE_ITERS && new_max + EPSILON_F * nice_step < data_max; ++iter)
      {
        nice_step = calcNiceStep(nice_step * NICE_BUMP_FACTOR);
        new_min = floorf(data_min / nice_step) * nice_step;
        new_max = new_min + nice_step * divs_f;
      }
    }

    const float new_span = new_max - new_min;
    if (!std::isfinite(new_min) || !std::isfinite(new_max) || new_span <= 0.0f || !std::isfinite(SPAN_INV_GUARD_F / new_span))
      return false;

    if (fabsf(new_min - _eff_y_min) > EPSILON_F || fabsf(new_max - _eff_y_max) > EPSILON_F)
    {
      _eff_y_min = new_min;
      _eff_y_max = new_max;
      return true;
    }

    return false;
  }

  void Chart::recalcLayout()
  {
    _cached_width = _width;
    _cached_height = _height;
    _cached_has_border = _has_border;

    int16_t x1{0};
    uint16_t w{0u};

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.calcTextBounds("Йg0", 0, 0, x1, _y_char_offset, w, _char_hgt);

    _display.setFont(_title_font);
    _display.setTextSize(_title_size);
    _display.calcTextBounds("Йg0", 0, 0, x1, _title_y_offset, w, _title_char_hgt);

    const int32_t border_pad = _has_border ? 1 : 0;
    const int32_t base_pad = border_pad + _padding;

    int32_t top = base_pad;
    int32_t bottom = base_pad;
    int32_t left = base_pad;
    const int32_t right = base_pad;

    if (!_title.isEmpty())
    {
      _title_y_loc = static_cast<uint16_t>(top);
      top += _title_char_hgt + ELEMENT_GAP;
    }

    bool has_named_series{false};
    if (_show_legend)
    {
      for (const Series& s : _series)
      {
        if (!s.name.isEmpty())
        {
          has_named_series = true;
          break;
        }
      }
    }

    if (has_named_series)
    {
      _legend_y_loc = static_cast<uint16_t>(top);
      top += _char_hgt + ELEMENT_GAP;
    }

    if (!_y_axis_name.isEmpty())
    {
      _y_name_y_loc = static_cast<uint16_t>(top);
      top += _char_hgt + ELEMENT_GAP;
    }

    if (!_x_axis_name.isEmpty())
      bottom += _char_hgt + ELEMENT_GAP;

    if (_show_x_labels)
      bottom += _char_hgt + TICK_LEN + LABEL_GAP;

    if (_show_y_labels)
    {
      const int32_t half_h_floor = static_cast<int32_t>(_char_hgt) / 2;
      const int32_t half_h_ceil = (static_cast<int32_t>(_char_hgt) + 1) / 2;
      top += half_h_floor;
      if (bottom < base_pad + half_h_ceil)
        bottom = base_pad + half_h_ceil;

      _display.setFont(_font);
      _display.setTextSize(_text_size);

      char buf[FMT_BUF_SIZE];
      int16_t y1{0};
      uint16_t h{0u};
      uint16_t max_y_lbl_w{0u};

      const float y_step = (_eff_y_max - _eff_y_min) / static_cast<float>(_grid_y_divs);
      for (uint8_t i{0u}; i <= _grid_y_divs; ++i)
      {
        float val = _eff_y_max - y_step * static_cast<float>(i);
        if (i == 0u)
          val = _eff_y_max;
        else if (i == _grid_y_divs)
          val = _eff_y_min;

        formatValue(buf, sizeof(buf), val, _y_decimals, _y_unit);
        _display.calcTextBounds(buf, 0, 0, x1, y1, w, h);
        if (w > max_y_lbl_w)
          max_y_lbl_w = w;
      }

      left += max_y_lbl_w + TICK_LEN + LABEL_GAP;
    }

    const int32_t avail_w = static_cast<int32_t>(_width) - left - right;
    const int32_t avail_h = static_cast<int32_t>(_height) - top - bottom;

    if (avail_w < static_cast<int32_t>(MIN_PLOT_SIZE) || avail_h < static_cast<int32_t>(MIN_PLOT_SIZE))
    {
      _plot_w = 0u;
      _plot_h = 0u;
      return;
    }

    _plot_x_loc = static_cast<uint16_t>(left);
    _plot_y_loc = static_cast<uint16_t>(top);
    _plot_w = static_cast<uint16_t>(avail_w);
    _plot_h = static_cast<uint16_t>(avail_h);
  }

  void Chart::onDraw()
  {
    if (_width != _cached_width || _height != _cached_height || _has_border != _cached_has_border)
    {
      _is_layout_dirty = true;
      _is_changed = true;
    }

    if (!_is_changed && !_is_data_changed)
      return;

    if (_visibility == INVISIBLE)
    {
      _is_changed = false;
      _is_data_changed = false;
      _has_drawn_once = false;
      hide();
      return;
    }

    if (_auto_scale_y && updateAutoScaleRange())
    {
      _is_changed = true;
      _is_layout_dirty = true;
    }

    const uint16_t cur_max_pts = calcMaxPoints();
    if (getEffectiveXScaleMode() == X_SCALE_FIT && _show_x_labels && _x_labels.empty() && _x_min >= _x_max && cur_max_pts != _last_max_points)
      _is_changed = true;

    _last_max_points = cur_max_pts;

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    // Швидкий шлях: оновилися лише дані, межі осей та розмітка незмінні
    if (!_is_changed && _is_data_changed && !_is_layout_dirty && !_is_transparent && _has_drawn_once)
    {
      _is_data_changed = false;
      if (_plot_w < MIN_PLOT_SIZE || _plot_h < MIN_PLOT_SIZE)
        return;

      fillPlotBackground(abs_x, abs_y);
      if (_show_grid)
        drawGrid(abs_x, abs_y);
      if (_show_zero_line)
        drawZeroLine(abs_x, abs_y);

      drawPlotContent(abs_x, abs_y);

      if (_show_axes)
        drawAxes(abs_x, abs_y);

      return;
    }

    _is_changed = false;
    _is_data_changed = false;

    if (_is_layout_dirty)
    {
      recalcLayout();
      _is_layout_dirty = false;
    }

    clear();
    drawTitleAndLegend(abs_x, abs_y);

    if (_plot_w < MIN_PLOT_SIZE || _plot_h < MIN_PLOT_SIZE)
    {
      _has_drawn_once = true;
      return;
    }

    if (!_is_transparent && _plot_back_color != _back_color)
      fillPlotBackground(abs_x, abs_y);

    if (_show_grid)
      drawGrid(abs_x, abs_y);
    if (_show_zero_line)
      drawZeroLine(abs_x, abs_y);

    drawPlotContent(abs_x, abs_y);

    if (_show_axes)
      drawAxes(abs_x, abs_y);

    drawAxisLabels(abs_x, abs_y);
    _has_drawn_once = true;
  }

  void Chart::fillPlotBackground(const uint16_t abs_x, const uint16_t abs_y) const
  {
    const int32_t px = static_cast<int32_t>(abs_x) + _plot_x_loc;
    const int32_t py = static_cast<int32_t>(abs_y) + _plot_y_loc;

#if CONFIG_IDF_TARGET_ESP32P4
    if (static_cast<uint32_t>(_plot_w) * _plot_h > PPA_FILL_SIZE_TRIGG)
    {
      const bool old_state = _display.isPPAEnabled();
      _display.setPPA(true);
      _display.fillRect(px, py, _plot_w, _plot_h, _plot_back_color);
      _display.setPPA(old_state);
      return;
    }
#endif

    _display.fillRect(px, py, _plot_w, _plot_h, _plot_back_color);
  }

  void Chart::drawTitleAndLegend(const uint16_t abs_x, const uint16_t abs_y) const
  {
    const int32_t border_pad = _has_border ? 1 : 0;
    const int32_t base_pad = border_pad + _padding;
    const int32_t content_w = static_cast<int32_t>(_width) - base_pad * 2;
    const int32_t content_h = static_cast<int32_t>(_height) - base_pad * 2;
    if (content_w <= 0 || content_h <= 0)
      return;

    const int16_t bound_x = static_cast<int16_t>(abs_x + base_pad);
    const int16_t bound_y = static_cast<int16_t>(abs_y + base_pad);
    const int16_t bound_w = static_cast<int16_t>(content_w);
    const int16_t bound_h = static_cast<int16_t>(content_h);
    const int32_t content_bottom = static_cast<int32_t>(abs_y) + _height - base_pad;

    int16_t x1{0};
    int16_t y1{0};
    uint16_t w{0u};
    uint16_t h{0u};

    if (!_title.isEmpty())
    {
      _display.setFont(_title_font);
      _display.setTextSize(_title_size);
      _display.setTextColor(_title_color);
      _display.calcTextBounds(_title.c_str(), 0, 0, x1, y1, w, h);

      int32_t tx = static_cast<int32_t>(abs_x) + (static_cast<int32_t>(_width) - static_cast<int32_t>(w)) / 2;
      if (tx < static_cast<int32_t>(abs_x) + base_pad)
        tx = static_cast<int32_t>(abs_x) + base_pad;

      _display.setTextBound(bound_x, bound_y, bound_w, bound_h);
      _display.setCursor(static_cast<int16_t>(tx), static_cast<int16_t>(static_cast<int32_t>(abs_y) + _title_y_loc - _title_y_offset));
      _display.print(_title.c_str());
      _display.resetTextBound();
    }

    _display.setFont(_font);
    _display.setTextSize(_text_size);

    if (_show_legend && !_series.empty())
    {
      const int32_t ly = static_cast<int32_t>(abs_y) + _legend_y_loc;
      if (ly + static_cast<int32_t>(_char_hgt) <= content_bottom)
      {
        _display.setTextColor(_legend_text_color);
        int32_t cur_x = static_cast<int32_t>(abs_x) + base_pad;
        const int32_t max_x = static_cast<int32_t>(abs_x) + _width - base_pad;
        const int32_t swatch_y = ly + (static_cast<int32_t>(_char_hgt) - LEGEND_SWATCH_SIZE) / 2;

        for (const Series& s : _series)
        {
          if (s.name.isEmpty())
            continue;

          _display.calcTextBounds(s.name.c_str(), 0, 0, x1, y1, w, h);
          const int32_t item_w = LEGEND_SWATCH_SIZE + LABEL_GAP + w;
          if (cur_x + item_w > max_x)
            break;

          if (s.visible)
            _display.fillRect(cur_x, swatch_y, LEGEND_SWATCH_SIZE, LEGEND_SWATCH_SIZE, s.color);
          else
            _display.drawRect(cur_x, swatch_y, LEGEND_SWATCH_SIZE, LEGEND_SWATCH_SIZE, s.color);

          _display.setTextBound(bound_x, bound_y, bound_w, bound_h);
          _display.setCursor(static_cast<int16_t>(cur_x + LEGEND_SWATCH_SIZE + LABEL_GAP), static_cast<int16_t>(ly - _y_char_offset));
          _display.print(s.name.c_str());
          _display.resetTextBound();
          cur_x += item_w + LEGEND_ITEM_GAP;
        }
      }
    }

    if (!_y_axis_name.isEmpty())
    {
      const int32_t ny = static_cast<int32_t>(abs_y) + _y_name_y_loc;
      if (ny + static_cast<int32_t>(_char_hgt) <= content_bottom)
      {
        _display.setTextColor(_text_color);
        _display.setTextBound(bound_x, bound_y, bound_w, bound_h);
        _display.setCursor(bound_x, static_cast<int16_t>(ny - _y_char_offset));
        _display.print(_y_axis_name.c_str());
        _display.resetTextBound();
      }
    }
  }

  void Chart::drawGrid(const uint16_t abs_x, const uint16_t abs_y) const
  {
    const int32_t px = static_cast<int32_t>(abs_x) + _plot_x_loc;
    const int32_t py = static_cast<int32_t>(abs_y) + _plot_y_loc;
    const int32_t pw = _plot_w;
    const int32_t ph = _plot_h;

    for (uint8_t i{0u}; i <= _grid_y_divs; ++i)
    {
      if (_show_axes && i == _grid_y_divs)
        continue;

      const int32_t gy = py + ((ph - 1) * i) / _grid_y_divs;
      if (_grid_dashed)
        drawDashedHLine(px, gy, pw, _grid_color);
      else
        _display.fillRect(px, gy, pw, 1, _grid_color);
    }

    for (uint8_t i{0u}; i <= _grid_x_divs; ++i)
    {
      if (_show_axes && i == 0u)
        continue;

      const int32_t gx = px + ((pw - 1) * i) / _grid_x_divs;
      if (_grid_dashed)
        drawDashedVLine(gx, py, ph, _grid_color);
      else
        _display.fillRect(gx, py, 1, ph, _grid_color);
    }
  }

  void Chart::drawZeroLine(const uint16_t abs_x, const uint16_t abs_y) const
  {
    if (_eff_y_min >= 0.0f || _eff_y_max <= 0.0f)
      return;

    const float y_span = _eff_y_max - _eff_y_min;
    if (!std::isfinite(y_span) || y_span <= 0.0f)
      return;

    const float inv_y_span = static_cast<float>(_plot_h - 1u) / y_span;
    if (!std::isfinite(inv_y_span))
      return;

    const int32_t px = static_cast<int32_t>(abs_x) + _plot_x_loc;
    const int32_t py = static_cast<int32_t>(abs_y) + _plot_y_loc;
    const int32_t zy = static_cast<int32_t>(floorf(valueToYFloat(0.0f, py, _plot_h, inv_y_span) + 0.5f));

    if (zy >= py && zy <= py + static_cast<int32_t>(_plot_h) - 1)
      _display.fillRect(px, zy, _plot_w, 1, _zero_line_color);
  }

  void Chart::drawAxes(const uint16_t abs_x, const uint16_t abs_y) const
  {
    const int32_t px = static_cast<int32_t>(abs_x) + _plot_x_loc;
    const int32_t py = static_cast<int32_t>(abs_y) + _plot_y_loc;

    _display.fillRect(px, py, 1, _plot_h, _axis_color);
    _display.fillRect(px, py + _plot_h - 1, _plot_w, 1, _axis_color);
  }

  void Chart::drawAxisLabels(const uint16_t abs_x, const uint16_t abs_y) const
  {
    const int32_t border_pad = _has_border ? 1 : 0;
    const int32_t base_pad = border_pad + _padding;
    const int32_t px = static_cast<int32_t>(abs_x) + _plot_x_loc;
    const int32_t py = static_cast<int32_t>(abs_y) + _plot_y_loc;
    const int32_t pw = _plot_w;
    const int32_t ph = _plot_h;

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextColor(_text_color);

    char buf[FMT_BUF_SIZE];
    int16_t x1{0};
    int16_t y1{0};
    uint16_t w{0u};
    uint16_t h{0u};

    if (_show_y_labels)
    {
      const float y_step = (_eff_y_max - _eff_y_min) / static_cast<float>(_grid_y_divs);
      const int32_t half_h = static_cast<int32_t>(_char_hgt) / 2;
      const int32_t min_lx = static_cast<int32_t>(abs_x) + border_pad;

      for (uint8_t i{0u}; i <= _grid_y_divs; ++i)
      {
        const int32_t gy = py + ((ph - 1) * i) / _grid_y_divs;
        if (_show_axes)
          _display.fillRect(px - TICK_LEN, gy, TICK_LEN, 1, _axis_color);

        float val = _eff_y_max - y_step * static_cast<float>(i);
        if (i == 0u)
          val = _eff_y_max;
        else if (i == _grid_y_divs)
          val = _eff_y_min;

        formatValue(buf, sizeof(buf), val, _y_decimals, _y_unit);
        _display.calcTextBounds(buf, 0, 0, x1, y1, w, h);

        int32_t lx = px - TICK_LEN - LABEL_GAP - static_cast<int32_t>(w);
        if (lx < min_lx)
          lx = min_lx;

        const int32_t ly = gy - half_h - _y_char_offset;
        _display.setCursor(static_cast<int16_t>(lx), static_cast<int16_t>(ly));
        _display.print(buf);
      }
    }

    if (_show_x_labels)
    {
      const int32_t tick_y = py + ph;
      const int32_t lbl_y = tick_y + TICK_LEN + LABEL_GAP - _y_char_offset;
      const int32_t plot_right = px + pw;
      const int32_t x_bound_h = static_cast<int32_t>(abs_y) + _height - border_pad - tick_y;
      int32_t last_drawn_right = px - 3;

      if (!_x_labels.empty())
      {
        const uint16_t lbl_count = static_cast<uint16_t>(_x_labels.size());
        for (uint16_t i{0u}; i < lbl_count; ++i)
        {
          int32_t gx{px};
          if (_chart_type == TYPE_BAR)
            gx = px + static_cast<int32_t>(floorf((static_cast<float>(pw) * (static_cast<float>(i) + 0.5f)) / static_cast<float>(lbl_count)));
          else if (lbl_count > 1u)
            gx = px + ((pw - 1) * i) / (lbl_count - 1u);
          else
            gx = px + pw / 2;

          if (_show_axes)
            _display.fillRect(gx, tick_y, 1, TICK_LEN, _axis_color);

          _display.calcTextBounds(_x_labels[i].c_str(), 0, 0, x1, y1, w, h);
          int32_t lx = gx - static_cast<int32_t>(w) / 2;
          if (lx + static_cast<int32_t>(w) > plot_right)
            lx = plot_right - static_cast<int32_t>(w);
          if (lx < px)
            lx = px;

          if (lx <= last_drawn_right + 2 || x_bound_h <= 0)
            continue;

          _display.setTextBound(static_cast<int16_t>(px), static_cast<int16_t>(tick_y), static_cast<int16_t>(pw), static_cast<int16_t>(x_bound_h));
          _display.setCursor(static_cast<int16_t>(lx), static_cast<int16_t>(lbl_y));
          _display.print(_x_labels[i].c_str());
          _display.resetTextBound();
          last_drawn_right = lx + w;
        }
      }
      else
      {
        float x_start = _x_min;
        float x_end = _x_max;
        if (x_end <= x_start)
        {
          x_start = 0.0f;
          const float eff_x_step = (_x_step > 0.0f) ? _x_step : 1.0f;
          const XScaleMode eff_mode = getEffectiveXScaleMode();
          if (eff_mode == X_SCALE_FIXED_STEP)
          {
            const float vis_steps = (_chart_type == TYPE_BAR) ? (static_cast<float>(pw) / static_cast<float>(_point_step_px)) : (static_cast<float>(pw - 1) / static_cast<float>(_point_step_px));
            x_end = ((vis_steps > 0.0f) ? vis_steps : 1.0f) * eff_x_step;
          }
          else if (eff_mode == X_SCALE_CAPACITY)
          {
            const uint16_t max_cap = calcMaxCapacity();
            const float cap_span = (_chart_type == TYPE_BAR) ? ((max_cap > 0u) ? static_cast<float>(max_cap) : 1.0f) : ((max_cap > 1u) ? static_cast<float>(max_cap - 1u) : 1.0f);
            x_end = cap_span * eff_x_step;
          }
          else
          {
            const float pts_span = (_chart_type == TYPE_BAR) ? ((_last_max_points > 0u) ? static_cast<float>(_last_max_points) : 1.0f) : ((_last_max_points > 1u) ? static_cast<float>(_last_max_points - 1u) : 1.0f);
            x_end = pts_span * eff_x_step;
          }

          if (!std::isfinite(x_end) || x_end <= 0.0f)
            x_end = 1.0f;
        }

        const float x_step = (x_end - x_start) / static_cast<float>(_grid_x_divs);
        for (uint8_t i{0u}; i <= _grid_x_divs; ++i)
        {
          const int32_t gx = px + ((pw - 1) * i) / _grid_x_divs;
          if (_show_axes)
            _display.fillRect(gx, tick_y, 1, TICK_LEN, _axis_color);

          float x_val = x_start + x_step * static_cast<float>(i);
          if (i == 0u)
            x_val = x_start;
          else if (i == _grid_x_divs)
            x_val = x_end;

          formatValue(buf, sizeof(buf), x_val, _x_decimals, _x_unit);
          _display.calcTextBounds(buf, 0, 0, x1, y1, w, h);

          int32_t lx = gx - static_cast<int32_t>(w) / 2;
          if (lx + static_cast<int32_t>(w) > plot_right)
            lx = plot_right - static_cast<int32_t>(w);
          if (lx < px)
            lx = px;

          if (lx <= last_drawn_right + 2 || x_bound_h <= 0)
            continue;

          _display.setTextBound(static_cast<int16_t>(px), static_cast<int16_t>(tick_y), static_cast<int16_t>(pw), static_cast<int16_t>(x_bound_h));
          _display.setCursor(static_cast<int16_t>(lx), static_cast<int16_t>(lbl_y));
          _display.print(buf);
          _display.resetTextBound();
          last_drawn_right = lx + w;
        }
      }
    }

    if (!_x_axis_name.isEmpty())
    {
      const int32_t content_w = static_cast<int32_t>(_width) - base_pad * 2;
      const int32_t content_h = static_cast<int32_t>(_height) - base_pad * 2;
      if (content_w > 0 && content_h > 0)
      {
        _display.calcTextBounds(_x_axis_name.c_str(), 0, 0, x1, y1, w, h);
        int32_t lx = px + (pw - static_cast<int32_t>(w)) / 2;
        if (lx < static_cast<int32_t>(abs_x) + base_pad)
          lx = static_cast<int32_t>(abs_x) + base_pad;

        const int32_t ly = static_cast<int32_t>(abs_y) + _height - base_pad - _char_hgt - _y_char_offset;
        _display.setTextBound(static_cast<int16_t>(abs_x + base_pad), static_cast<int16_t>(abs_y + base_pad), static_cast<int16_t>(content_w), static_cast<int16_t>(content_h));
        _display.setCursor(static_cast<int16_t>(lx), static_cast<int16_t>(ly));
        _display.print(_x_axis_name.c_str());
        _display.resetTextBound();
      }
    }
  }

  void Chart::drawPlotContent(const uint16_t abs_x, const uint16_t abs_y) const
  {
    const uint16_t max_pts = calcMaxPoints();
    const float y_span = _eff_y_max - _eff_y_min;
    if (max_pts == 0u || !std::isfinite(y_span) || y_span <= 0.0f)
      return;

    const int32_t px = static_cast<int32_t>(abs_x) + _plot_x_loc;
    const int32_t py = static_cast<int32_t>(abs_y) + _plot_y_loc;
    const int32_t pw = _plot_w;
    const int32_t ph = _plot_h;
    const float inv_y_span = static_cast<float>(ph - 1) / y_span;
    if (!std::isfinite(inv_y_span))
      return;

    const uint16_t max_cap = calcMaxCapacity();
    if (_chart_type == TYPE_BAR)
    {
      int32_t base_y = py + ph - 1;
      if (_eff_y_min < 0.0f && _eff_y_max > 0.0f)
        base_y = static_cast<int32_t>(floorf(valueToYFloat(0.0f, py, ph, inv_y_span) + 0.5f));
      else if (_eff_y_max <= 0.0f)
        base_y = py;

      drawBarSeries(px, py, pw, ph, max_pts, max_cap, inv_y_span, base_y);
      return;
    }

    const uint16_t denom_pts = (getEffectiveXScaleMode() == X_SCALE_CAPACITY) ? max_cap : max_pts;

    for (const Series& s : _series)
    {
      if (s.visible && s.count > 0u)
        drawLineLikeSeries(s, px, py, pw, ph, denom_pts, inv_y_span);
    }
  }

  void Chart::drawLineLikeSeries(
      const Series& series,
      const int32_t px,
      const int32_t py,
      const int32_t pw,
      const int32_t ph,
      const uint16_t denom_pts,
      const float inv_y_span) const
  {
    const bool is_fixed_step = (getEffectiveXScaleMode() == X_SCALE_FIXED_STEP);
    const bool is_phys_step = isPhysicalXStepActive();
    const uint16_t start_idx = (is_fixed_step || is_phys_step) ? calcVisibleStartIdx(series.count, pw) : 0u;
    const uint16_t vis_pts = series.count - start_idx;
    const int32_t x_max_px = px + pw - 1;
    const float py_f = static_cast<float>(py);
    const float y_max_f = static_cast<float>(py + ph - 1);

    float x_step{0.0f};
    if (is_phys_step)
    {
      x_step = static_cast<float>(pw - 1) * (_x_step / (_x_max - _x_min));
      if (!std::isfinite(x_step) || x_step > static_cast<float>(pw))
        x_step = static_cast<float>(pw);
    }
    else if (!is_fixed_step && denom_pts > 1u)
    {
      x_step = static_cast<float>(pw - 1) / static_cast<float>(denom_pts - 1u);
    }

    bool has_prev{false};
    int32_t prev_x{0};
    float prev_y_f{0.0f};

    for (uint16_t i{start_idx}; i < series.count; ++i)
    {
      const float v = getSeriesValue(series, i);
      if (!std::isfinite(v))
      {
        has_prev = false;
        continue;
      }

      int32_t cur_x{px};
      if (is_fixed_step)
        cur_x = px + static_cast<int32_t>(i - start_idx) * static_cast<int32_t>(_point_step_px);
      else if (is_phys_step)
        cur_x = px + static_cast<int32_t>(floorf(static_cast<float>(i - start_idx) * x_step + 0.5f));
      else if (denom_pts > 1u)
        cur_x = px + static_cast<int32_t>(floorf(static_cast<float>(i) * x_step + 0.5f));
      else
        cur_x = px + pw / 2;

      if (cur_x > x_max_px)
        cur_x = x_max_px;

      const float cur_y_f = valueToYFloat(v, py, ph, inv_y_span);

      if (_chart_type != TYPE_SCATTER && has_prev)
      {
        if (_chart_type == TYPE_STEP)
        {
          drawClippedLine(prev_x, prev_y_f, cur_x, prev_y_f, px, py, pw, ph, series.color);
          drawClippedLine(cur_x, prev_y_f, cur_x, cur_y_f, px, py, pw, ph, series.color);
        }
        else
        {
          drawClippedLine(prev_x, prev_y_f, cur_x, cur_y_f, px, py, pw, ph, series.color);
        }
      }

      if ((_chart_type == TYPE_SCATTER || _show_markers || vis_pts == 1u) && cur_y_f >= py_f && cur_y_f <= y_max_f)
      {
        const int32_t cur_y = static_cast<int32_t>(floorf(cur_y_f + 0.5f));
        drawClippedMarker(cur_x, cur_y, px, py, pw, ph, series.color);
      }

      prev_x = cur_x;
      prev_y_f = cur_y_f;
      has_prev = true;
    }
  }

  void Chart::drawBarSeries(
      const int32_t px,
      const int32_t py,
      const int32_t pw,
      const int32_t ph,
      const uint16_t max_pts,
      const uint16_t max_cap,
      const float inv_y_span,
      const int32_t base_y) const
  {
    uint16_t vis_count{0u};
    for (const Series& s : _series)
    {
      if (s.visible)
        ++vis_count;
    }

    if (vis_count == 0u)
      return;

    const XScaleMode eff_mode = getEffectiveXScaleMode();
    uint16_t start_idx{0u};
    float slot_w{static_cast<float>(pw)};

    if (eff_mode == X_SCALE_FIXED_STEP)
    {
      start_idx = calcVisibleStartIdx(max_pts, pw);
      slot_w = (pw >= static_cast<int32_t>(_point_step_px)) ? static_cast<float>(_point_step_px) : static_cast<float>(pw);
    }
    else if (isPhysicalXStepActive())
    {
      start_idx = calcVisibleStartIdx(max_pts, pw);
      slot_w = static_cast<float>(pw) * (_x_step / (_x_max - _x_min));
      if (!std::isfinite(slot_w) || slot_w > static_cast<float>(pw))
        slot_w = static_cast<float>(pw);
    }
    else
    {
      const uint16_t slot_count = (eff_mode == X_SCALE_CAPACITY) ? max_cap : max_pts;
      if (slot_count == 0u)
        return;

      slot_w = static_cast<float>(pw) / static_cast<float>(slot_count);
    }

    const int32_t slot_px = static_cast<int32_t>(slot_w);
    const int32_t usable_slot = slot_px - static_cast<int32_t>(_bar_gap);
    const int32_t inner_gap = (vis_count > 1u && usable_slot > static_cast<int32_t>(vis_count) * 2) ? 1 : 0;
    int32_t bar_w = (usable_slot - inner_gap * static_cast<int32_t>(vis_count - 1u)) / static_cast<int32_t>(vis_count);
    if (bar_w < 1)
      bar_w = 1;

    const int32_t group_w = bar_w * static_cast<int32_t>(vis_count) + inner_gap * static_cast<int32_t>(vis_count - 1u);
    const int32_t x_max_px = px + pw - 1;
    const int32_t y_max_px = py + ph - 1;
    const float py_f = static_cast<float>(py);
    const float y_max_f = static_cast<float>(y_max_px);

    for (uint16_t pt{start_idx}; pt < max_pts; ++pt)
    {
      const uint16_t local_pt = pt - start_idx;
      const int32_t slot_start = px + static_cast<int32_t>(static_cast<float>(local_pt) * slot_w);
      if (slot_start > x_max_px)
        break;

      int32_t cur_x = slot_start + (slot_px - group_w) / 2;
      if (cur_x < px)
        cur_x = px;

      for (const Series& s : _series)
      {
        if (!s.visible)
          continue;

        if (cur_x > x_max_px)
          break;

        if (pt < s.count)
        {
          const float v = getSeriesValue(s, pt);
          const bool out_below = (_eff_y_min >= 0.0f && v < _eff_y_min);
          const bool out_above = (_eff_y_max <= 0.0f && v > _eff_y_max);

          if (std::isfinite(v) && !out_below && !out_above)
          {
            float vy_f = valueToYFloat(v, py, ph, inv_y_span);
            if (vy_f < py_f)
              vy_f = py_f;
            else if (vy_f > y_max_f)
              vy_f = y_max_f;

            const int32_t vy = static_cast<int32_t>(floorf(vy_f + 0.5f));
            const int32_t top_y = (vy < base_y) ? vy : base_y;
            int32_t bar_h = (vy < base_y) ? (base_y - vy + 1) : (vy - base_y + 1);
            if (top_y + bar_h > py + ph)
              bar_h = py + ph - top_y;

            int32_t clipped_w = bar_w;
            if (cur_x + clipped_w - 1 > x_max_px)
              clipped_w = x_max_px - cur_x + 1;

            if (clipped_w > 0 && bar_h > 0)
              _display.fillRect(cur_x, top_y, clipped_w, bar_h, s.color);
          }
        }

        cur_x += bar_w + inner_gap;
      }
    }
  }

  void Chart::drawClippedLine(
      const int32_t x0_in,
      float y0_f,
      const int32_t x1_in,
      float y1_f,
      const int32_t px,
      const int32_t py,
      const int32_t pw,
      const int32_t ph,
      const uint16_t color) const
  {
    const int32_t x_max = px + pw - 1;
    const int32_t y_max = py + ph - 1;
    const float py_f = static_cast<float>(py);
    const float y_max_f = static_cast<float>(y_max);

    if ((y0_f < py_f && y1_f < py_f) || (y0_f > y_max_f && y1_f > y_max_f))
      return;

    float x0_f = static_cast<float>(x0_in);
    float x1_f = static_cast<float>(x1_in);

    if (y0_f < py_f)
    {
      x0_f += (x1_f - x0_f) * (py_f - y0_f) / (y1_f - y0_f);
      y0_f = py_f;
    }
    else if (y0_f > y_max_f)
    {
      x0_f += (x1_f - x0_f) * (y_max_f - y0_f) / (y1_f - y0_f);
      y0_f = y_max_f;
    }

    if (y1_f < py_f)
    {
      x1_f += (x0_f - x1_f) * (py_f - y1_f) / (y0_f - y1_f);
      y1_f = py_f;
    }
    else if (y1_f > y_max_f)
    {
      x1_f += (x0_f - x1_f) * (y_max_f - y1_f) / (y0_f - y1_f);
      y1_f = y_max_f;
    }

    int32_t x0 = static_cast<int32_t>(floorf(x0_f + 0.5f));
    int32_t x1 = static_cast<int32_t>(floorf(x1_f + 0.5f));
    const int32_t y0 = static_cast<int32_t>(floorf(y0_f + 0.5f));
    const int32_t y1 = static_cast<int32_t>(floorf(y1_f + 0.5f));

    if (x0 < px)
      x0 = px;
    else if (x0 > x_max)
      x0 = x_max;

    if (x1 < px)
      x1 = px;
    else if (x1 > x_max)
      x1 = x_max;

    const int32_t start_off = -static_cast<int32_t>((_line_thickness - 1u) / 2u);
    const int32_t end_off = start_off + static_cast<int32_t>(_line_thickness) - 1;

    if (y0 == y1)
    {
      const int32_t min_x = (x0 < x1) ? x0 : x1;
      const int32_t max_x = (x0 < x1) ? x1 : x0;
      int32_t sy = y0 + start_off;
      int32_t ey = y0 + end_off;
      if (sy < py)
        sy = py;
      if (ey > y_max)
        ey = y_max;
      if (sy <= ey)
        _display.fillRect(min_x, sy, max_x - min_x + 1, ey - sy + 1, color);
      return;
    }

    if (x0 == x1)
    {
      const int32_t min_y = (y0 < y1) ? y0 : y1;
      const int32_t max_y = (y0 < y1) ? y1 : y0;
      int32_t sx = x0 + start_off;
      int32_t ex = x0 + end_off;
      if (sx < px)
        sx = px;
      if (ex > x_max)
        ex = x_max;
      if (sx <= ex)
        _display.fillRect(sx, min_y, ex - sx + 1, max_y - min_y + 1, color);
      return;
    }

    if (_line_thickness <= 1u)
    {
      _display.drawLine(static_cast<int16_t>(x0), static_cast<int16_t>(y0), static_cast<int16_t>(x1), static_cast<int16_t>(y1), color);
      return;
    }

    const int32_t dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    const int32_t dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);

    if (dx >= dy)
    {
      for (int32_t off = start_off; off <= end_off; ++off)
      {
        const int32_t oy0 = y0 + off;
        const int32_t oy1 = y1 + off;
        if (oy0 >= py && oy0 <= y_max && oy1 >= py && oy1 <= y_max)
          _display.drawLine(static_cast<int16_t>(x0), static_cast<int16_t>(oy0), static_cast<int16_t>(x1), static_cast<int16_t>(oy1), color);
      }
    }
    else
    {
      for (int32_t off = start_off; off <= end_off; ++off)
      {
        const int32_t ox0 = x0 + off;
        const int32_t ox1 = x1 + off;
        if (ox0 >= px && ox0 <= x_max && ox1 >= px && ox1 <= x_max)
          _display.drawLine(static_cast<int16_t>(ox0), static_cast<int16_t>(y0), static_cast<int16_t>(ox1), static_cast<int16_t>(y1), color);
      }
    }
  }

  void Chart::drawClippedMarker(
      const int32_t cx,
      const int32_t cy,
      const int32_t px,
      const int32_t py,
      const int32_t pw,
      const int32_t ph,
      const uint16_t color) const
  {
    const int32_t r = _marker_radius;
    const int32_t x_max = px + pw - 1;
    const int32_t y_max = py + ph - 1;

    if (cx - r >= px && cx + r <= x_max && cy - r >= py && cy + r <= y_max)
    {
      _display.fillCircle(static_cast<int16_t>(cx), static_cast<int16_t>(cy), static_cast<int16_t>(r), color);
      return;
    }

    const int32_t r_sq = r * r;
    for (int32_t dy = -r; dy <= r; ++dy)
    {
      const int32_t y = cy + dy;
      if (y < py || y > y_max)
        continue;

      int32_t dx = r;
      const int32_t dy_sq = dy * dy;
      while (dx > 0 && dx * dx + dy_sq > r_sq)
        --dx;

      int32_t x0 = cx - dx;
      int32_t x1 = cx + dx;
      if (x0 < px)
        x0 = px;
      if (x1 > x_max)
        x1 = x_max;

      if (x0 <= x1)
        _display.fillRect(x0, y, x1 - x0 + 1, 1, color);
    }
  }

  void Chart::drawDashedHLine(const int32_t x, const int32_t y, const int32_t len, const uint16_t color) const
  {
    for (int32_t off{0}; off < len; off += DASH_STEP)
    {
      const int32_t seg = (off + DASH_LEN <= len) ? DASH_LEN : (len - off);
      _display.fillRect(x + off, y, seg, 1, color);
    }
  }

  void Chart::drawDashedVLine(const int32_t x, const int32_t y, const int32_t len, const uint16_t color) const
  {
    for (int32_t off{0}; off < len; off += DASH_STEP)
    {
      const int32_t seg = (off + DASH_LEN <= len) ? DASH_LEN : (len - off);
      _display.fillRect(x, y + off, 1, seg, color);
    }
  }

  float Chart::valueToYFloat(const float value, const int32_t py, const int32_t ph, const float inv_y_span) const
  {
    const float y_f = static_cast<float>(py + ph - 1) - (value - _eff_y_min) * inv_y_span;
    if (y_f < -MAX_COORD_F)
      return -MAX_COORD_F;
    if (y_f > MAX_COORD_F)
      return MAX_COORD_F;

    return y_f;
  }

  void Chart::formatValue(char* out_buf, const size_t buf_size, const float value, const uint8_t decimals, const String& unit) const
  {
    const float clean_val = (fabsf(value) < EPSILON_F) ? 0.0f : value;

    if (fabsf(clean_val) >= SCI_FMT_THRESHOLD_F)
      snprintf(out_buf, buf_size, "%.1e%s", clean_val, unit.c_str());
    else if (decimals == 0u)
      snprintf(out_buf, buf_size, "%ld%s", static_cast<long>(lroundf(clean_val)), unit.c_str());
    else
      snprintf(out_buf, buf_size, "%.*f%s", static_cast<int>(decimals), clean_val, unit.c_str());
  }
}  // namespace pixeler
