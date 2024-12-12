/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "meter.hpp"
#include "ui_plotter.hpp"
#include <M5Utility.hpp>
#include <M5Unified.h>

class EMA {
public:
    explicit EMA(float factor = 0.92f) : _alpha(factor)
    {
    }

    inline void clear()
    {
        _ema_value = std::numeric_limits<float>::quiet_NaN();
    }

    inline float update(float new_value)
    {
        if (!std::isnan(_ema_value)) {
            _ema_value = _alpha * new_value + (1.0f - _alpha) * _ema_value;
        } else {
            _ema_value = new_value;
        }
        return _ema_value;
    }

private:
    float _alpha{0.92f}, _ema_value{std::numeric_limits<float>::quiet_NaN()};
};

namespace {
m5::ui::Plotter* plotter{};
EMA ema{};
m5gfx::rgb565_t theme_color{};
}  // namespace

void initialize_meter(const uint32_t swid, const uint32_t shgt, const uint32_t elements,
                      const m5gfx::rgb565_t theme_clr)
{
    delete plotter;

    auto hgt = shgt - 32;
    plotter  = new m5::ui::Plotter(nullptr, elements, swid, hgt, 100);
    plotter->setGaugeTextDatum(textdatum_t::top_right);
    plotter->setLineColor(theme_clr);
    ema.clear();
    theme_color = theme_clr;
}

void store_value(const float val)
{
    plotter->push_back(ema.update(val));
    //    plotter->push_back(val);
}

void draw_meter(LGFX_Sprite& spr, const int32_t offset, const char* tag, const char* unit)
{
    plotter->push(&spr, 0, offset);

    int32_t x{}, y{plotter->height() + offset};
    int32_t wid{spr.width()}, hgt{32};
    auto f  = spr.getFont();
    auto td = spr.getTextDatum();

    spr.fillRect(x, y, wid, 4, theme_color);

    spr.setFont(wid >= 240 ? &fonts::FreeSansBold12pt7b : &fonts::FreeSansBold9pt7b);

    spr.setTextDatum(textdatum_t::middle_left);
    spr.drawString(tag, x, y + 32 / 2);

    auto s = m5::utility::formatString("%.2f", 0.01f * plotter->latest());
    spr.setTextDatum(textdatum_t::middle_right);
    spr.drawString(s.c_str(), x + wid, y + 32 / 2);

    spr.setTextDatum(td);
    spr.setFont(f);
}
