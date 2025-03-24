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
// #define USING_DISPLAY_MODULE

#if defined(USING_DISPLAY_MODULE)
#include <M5ModuleDisplay.h>
#endif
#include <M5Unified.h>
#include <M5GFX.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
#include "../src/meter.hpp"
#include <cassert>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_VMETER) && !defined(USING_UNIT_AMETER) && !defined(USING_UNIT_KMETER_ISO) && \
    !defined(USING_UNIT_DUAL_KMETER)
// For Vmeter
// #define USING_UNIT_VMETER
// For Ameter
// #define USING_UNIT_AMETER
// For KmeterISO
// #define USING_UNIT_KMETER_ISO
// For DualKmeter
// #define USING_UNIT_DUAL_KMETER
#endif

namespace {
auto& lcd = M5.Display;
LGFX_Sprite strips[2];
constexpr uint32_t SPLIT_NUM{4};
int32_t strip_height{};
m5::unit::UnitUnified Units;

#if defined(USING_UNIT_VMETER)
#pragma message "Using Vmeter"
m5::unit::UnitVmeter unit{};
constexpr char tag[]      = "Voltage";
constexpr char val_unit[] = "V";
constexpr m5gfx::rgb565_t theme_clr{48, 144, 224};
#elif defined(USING_UNIT_AMETER)
#pragma message "Using Ameter"
m5::unit::UnitAmeter unit{};
constexpr char tag[]      = "Current";
constexpr char val_unit[] = "A";
constexpr m5gfx::rgb565_t theme_clr{255, 80, 144};
#elif defined(USING_UNIT_KMETER_ISO)
m5::unit::UnitKmeterISO unit{};
#pragma message "Using KmeterISO"
constexpr char tag[]      = "Temp";
constexpr char val_unit[] = "C";
constexpr m5gfx::rgb565_t theme_clr{240, 188, 104};
#elif defined(USING_UNIT_DUAL_KMETER)
m5::unit::UnitDualKmeter unit{0x11};  // Configured address
#pragma message "Using DualKmeter"
const char* tag_table[]   = {"Temp1", "Temp2"};
const char* tag           = tag_table[0];
constexpr char val_unit[] = "C";
constexpr m5gfx::rgb565_t theme_clr{240, 188, 104};
#else
#error "Choose unit"
#endif

volatile SemaphoreHandle_t _updateLock{};

void update_meter(void*)
{
    for (;;) {
        M5.update();
#if defined(USING_UNIT_DUAL_KMETER)
        auto touch = M5.Touch.getDetail();
        // Togle channel 1 <-> 2
        if (M5.BtnA.wasClicked() || touch.wasClicked()) {
            if (xSemaphoreTake(_updateLock, 0)) {
                using namespace m5::unit::dual_kmeter;
                static Channel ch{};
                ch = (ch == Channel::One) ? Channel::Two : Channel::One;
                if (unit.writeCurrentChannel(ch)) {
                    M5.Speaker.tone(3000, 20);
                    m5::utility::delay(50);
                    M5.Speaker.tone(2000, 20);
                    tag = tag_table[m5::stl::to_underlying(ch)];
                    clear_meter();
                }
                xSemaphoreGive(_updateLock);
                delay(10);
                continue;
            }
        }
#endif
        Units.update();
        if (xSemaphoreTake(_updateLock, 1)) {
            while (unit.available()) {
#if defined(USING_UNIT_VMETER)
                store_value(unit.voltage());
#elif defined(USING_UNIT_AMETER)
                store_value(unit.current());
#elif defined(USING_UNIT_KMETER_ISO) || defined(USING_UNIT_DUAL_KMETER)
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
    auto m5cfg = M5.config();
#if defined(__M5GFX_M5MODULEDISPLAY__)
    m5cfg.module_display.logical_width  = 320;
    m5cfg.module_display.logical_height = 240;
#endif

#if defined(USING_UNIT_DUAL_KMETER)
    // Disable because it conflicts with internal i2c
    m5cfg.internal_imu = false;  // Disable internal IMU
    m5cfg.internal_rtc = false;  // Disable internal RTC
#endif
    M5.begin(m5cfg);

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

#if defined(__M5GFX_M5MODULEDISPLAY__)
    // Choose Display if Display module and cable connected
    int32_t idx = M5.getDisplayIndex(m5gfx::board_M5ModuleDisplay);
    M5_LOGI("ModuleDisplay?:%d", idx);
    if (idx >= 0) {
        uint8_t buf[256];
        if (0 < ((lgfx::Panel_M5HDMI*)(M5.Displays(idx).panel()))->readEDID(buf, sizeof(buf))) {
            M5_LOGI("Detected the display, Set Display primary");
            M5.setPrimaryDisplay(idx);
        }
    }
#endif

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
#if defined(USING_UNIT_DUAL_KMETER)
    auto pin_num_sda = M5.getPin(m5::pin_name_t::in_i2c_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::in_i2c_scl);
#else
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
#endif
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    auto cfg = unit.config();

#if defined(USING_UNIT_VMETER) || defined(USING_UNIT_AMETER)
    cfg.rate = m5::unit::ads111x::Sampling::Rate250;
#elif defined(USING_UNIT_KMETER_ISO) || defined(USING_UNIT_DUAL_KMETER)
    cfg.interval = 20;
#else
#error "Choose unit"
#endif

    unit.config(cfg);

    auto ccfg        = unit.component_config();
    ccfg.stored_size = 250;
    unit.component_config(ccfg);

    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
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
