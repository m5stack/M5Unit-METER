/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Graphical meter example for Unit-Meter

  The core must be equipped with LCD
  Ameter,Vmeter,or KmeterISO unit must be connected
*/
#include <M5Unified.h>
#include <M5GFX.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
#include "meter.hpp"
#include <cassert>

// ********************************************************************************
// *** Select the unit (override this source or specify in the compile options) ***
// ********************************************************************************
#if !defined(USING_VMETER) && !defined(USING_AMETER) && !defined(USING_KMETER_ISO)
#define USING_VMETER
// #define USING_AMETER
// #define USING_KMETER_ISO
#endif

namespace {
auto& lcd = M5.Display;
LGFX_Sprite strips[2];
constexpr uint32_t SPLIT_NUM{4};
int32_t strip_height{};

m5::unit::UnitUnified Units;
#if defined(USING_VMETER)
#pragma message "Using Vmeter"
m5::unit::UnitVmeter unit{};
constexpr char tag[]      = "Voltage";
constexpr char val_unit[] = "V";
constexpr m5gfx::rgb565_t theme_clr{45, 136, 218};
#elif defined(USING_AMETER)
#pragma message "Using Ameter"
m5::unit::UnitAmeter unit{};
constexpr char tag[]      = "Current";
constexpr char val_unit[] = "A";
constexpr m5gfx::rgb565_t theme_clr{254, 79, 147};
#elif defined(USING_KMETER_ISO)
m5::unit::UnitKmeterISO unit{};
#pragma message "Using KmeterISO"
constexpr char tag[]      = "Temp";
constexpr char val_unit[] = "C";
constexpr m5gfx::rgb565_t theme_clr{241, 188, 105};
#else
#error "Choose unit"
#endif

volatile SemaphoreHandle_t _updateLock{};

void update_meter(void*)
{
    for (;;) {
        Units.update();
        if (xSemaphoreTake(_updateLock, 1)) {
            while (unit.available()) {
#if defined(USING_VMETER)
                store_value(unit.voltage());
#elif defined(USING_AMETER)
                store_value(unit.current());
#elif defined(USING_KMETER_ISO)
                store_value(unit.temperature());
#else
#error "Choose unit"
#endif
                unit.discard();
            }
            xSemaphoreGive(_updateLock);
        }
    }
}

}  // namespace

void setup()
{
    M5.begin();

    // No LCD or display device?
    if (lcd.width() == 0 || lcd.isEPD()) {
        M5_LOGE("The core must be equipped with LCD");
        M5.Speaker.tone(1000, 20);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // Make strips
    strip_height = (lcd.height() + SPLIT_NUM - 1) / SPLIT_NUM;
    uint32_t cnt{};
    for (auto&& spr : strips) {
        spr.setPsram(false);
        spr.setColorDepth(lcd.getColorDepth());
        cnt += spr.createSprite(lcd.width(), strip_height) ? 1 : 0;
        spr.setAddrWindow(0, 0, lcd.width(), strip_height);
    }
    assert(cnt == 2 && "Failed to create sprite");

    // UnitUnified
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    auto cfg = unit.config();
#if defined(USING_VMETER) || defined(USING_AMETER)
    cfg.rate = m5::unit::ads111x::Sampling::Rate250;
#elif defined(USING_KMETER_ISO)
    cfg.interval = 1000 / 250;
#else
#error "Choose unit"
#endif
    unit.config(cfg);
    auto ccfg        = unit.component_config();
    ccfg.stored_size = 250;
    unit.component_config(ccfg);

    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    initialize_meter(lcd.width(), lcd.height(), lcd.width(), theme_clr);

    lcd.setAddrWindow(0, 0, lcd.width(), lcd.height());
    lcd.startWrite();

    _updateLock = xSemaphoreCreateBinary();
    xSemaphoreGive(_updateLock);
    xTaskCreateUniversal(update_meter, "meter", 8192, nullptr, 2, nullptr, APP_CPU_NUM);
}

void loop()
{
#if 0
    static uint32_t fpsCnt{}, fps{};
    static unsigned long start_at{m5::utility::millis()};

    auto now = m5::utility::millis();
    if (now >= start_at + 1000) {
        fps = fpsCnt;
        M5_LOGW("loop FPS:%u", fps);
        fpsCnt   = 0;
        start_at = now;
    }
#endif

    M5.update();

    // Draw strips with DMA transfer
    while (lcd.dmaBusy()) {
        m5::utility::delay(1);
    }

    do {
        if (xSemaphoreTake(_updateLock, 0)) {
            //++fpsCnt;

            static uint32_t current{};
            int32_t offset{};
            uint32_t cnt{SPLIT_NUM};
            while (cnt--) {
                auto& spr = strips[current];
                spr.clear();
                draw_meter(spr, offset, tag, val_unit);
                spr.pushSprite(&lcd, 0, -offset);

                current ^= 1;
                offset -= strip_height;
            }
            xSemaphoreGive(_updateLock);
        }
    } while (!lcd.dmaBusy());
}
