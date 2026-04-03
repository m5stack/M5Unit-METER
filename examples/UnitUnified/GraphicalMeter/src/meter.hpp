/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef M5_UNIT_METER_EXAMPLE_GRAPHICAL_METER_METER_HPP
#define M5_UNIT_METER_EXAMPLE_GRAPHICAL_METER_METER_HPP

#include <cstdint>
#include <M5GFX.h>

enum : uint8_t {
    PaletteBackground = 0,
    PaletteGauge      = 1,
    PaletteTheme      = 2,
    PaletteText       = 3,
};

void initialize_meter(const uint32_t swid, const uint32_t shgt, const uint32_t elements,
                      const m5gfx::rgb565_t theme_clr);
void store_value(const float val);
void clear_meter();
void draw_meter(LGFX_Sprite& spr, const int32_t offset, const char* tag = "", const char* unit = "");
#endif
