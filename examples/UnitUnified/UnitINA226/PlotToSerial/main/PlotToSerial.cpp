/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example: UnitINA226
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_INA226_1A) && !defined(USING_UNIT_INA226_10A) && !defined(BUILTIN_UNIT_INA226_10A)
// For UnitINA226-1A (U200-1A)
// #define USING_UNIT_INA226_1A
// For UnitINA226-10A (U200)
// #define USING_UNIT_INA226_10A
// For Tab5 built-in INA226 (10A)
// #define BUILTIN_UNIT_INA226_10A
#endif

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
#if defined(USING_UNIT_INA226_1A)

#pragma message "Using 1A"
m5::unit::UnitINA226_1A unit;

#elif defined(USING_UNIT_INA226_10A)

#pragma message "Using 10A"
m5::unit::UnitINA226_10A unit;

#elif defined(BUILTIN_UNIT_INA226_10A)

#pragma message "Using 10A (Tab5)"
m5::unit::UnitINA226_10A unit;
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
#error "Compile not for Tab5"
#endif

#else
#error "Choose unit"
#endif
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(BUILTIN_UNIT_INA226_10A)
    auto board = M5.getBoard();
    if (board != m5::board_t::board_M5Tab5) {
        M5_LOGE("Core is NOT Tab5");
        m5::unit::wiring::failStop();
    }

    if (!Units.add(unit, M5.In_I2C) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

#else
    // NessoN1 -> SoftwareI2C (M5HAL), NanoC6 / NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }
#endif

    lcd.setFont(&fonts::AsciiFont8x16);

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);
    lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
}

void loop()
{
    using namespace m5::unit::ina226;

    M5.update();
    Units.update();

    if (unit.updated()) {
        M5.Log.printf(">A:%f\n>SV:%f\n>BV:%f\n>W:%f\n", unit.current(), unit.shuntVoltage(), unit.voltage(),
                      unit.power());

#if false && defined(BUILTIN_UNIT_INA226_10A)
        // Compare with M5Unified M5.Power.Ina226 values
        M5.Log.printf(">M5_BV:%f\n>M5_SV:%f\n>M5_A:%f\n>M5_W:%f\n", M5.Power.Ina226.getBusVoltage(),
                      M5.Power.Ina226.getShuntVoltage(), M5.Power.Ina226.getShuntCurrent(),
                      M5.Power.Ina226.getPower());
#endif

        lcd.startWrite();
        lcd.setCursor(0, 0);
        lcd.printf(
            " C:%5.2f mA\n"
            "SV:%5.2f mV\n"
            "BV:%5.2f mV\n"
            " P:%5.2f mW",
            unit.current(), unit.shuntVoltage(), unit.voltage(), unit.power());
        lcd.endWrite();
    }

    if (M5.BtnA.wasClicked()) {
        static bool single{};
        single = !single;
        if (single) {
            M5.Speaker.tone(1500, 20);
            Data d{};
            unit.stopPeriodicMeasurement();
            if (unit.measureSingleshot(d)) {
                M5.Log.printf("Single:A:%f SV:%f BV:%f W:%f\n", d.current(), d.shuntVoltage(), d.voltage(), d.power());
            } else {
                M5_LOGE("Failed to measureSingleshot");
            }
        } else {
            M5.Speaker.tone(2500, 20);
            M5.Log.printf("Start periodic\n");
            unit.startPeriodicMeasurement();
        }
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
