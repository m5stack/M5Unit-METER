/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDualKmeter
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)
namespace {
m5::unit::UnitUnified Units;
m5::unit::UnitDualKmeter unit{0x11};  // Configured address
auto& lcd = M5.Display;
}  // namespace

using namespace m5::unit::dual_kmeter;

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // DualKmeter uses M5-Bus internal I2C
    if (!Units.add(unit, M5.In_I2C) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        auto d = unit.oldest();
        M5.Log.printf(">Temperature%d:%.2f\n", (int)d.channel + 1, d.temperature());
    }

    // Toggle single <-> periodic
    if (M5.BtnA.wasClicked()) {
        static bool single{};
        single = !single;
        if (single) {
            M5.Speaker.tone(2000, 20);
            unit.stopPeriodicMeasurement();

            Data d{};
            if (unit.measureSingleshot(d, unit.measurementChannel())) {
                M5.Log.printf("Single temperature %.2f\n", d.temperature());
            }
            if (unit.measureInternalSingleshot(d, unit.measurementChannel())) {
                M5.Log.printf("Single:internal temperature %.2f\n", d.temperature());
            }
            lcd.fillScreen(TFT_BLUE);
        } else {
            M5.Speaker.tone(3000, 40);
            unit.startPeriodicMeasurement();
            lcd.fillScreen(TFT_DARKGREEN);
        }
    }

    // Toggle channel 1 <-> 2
    if (M5.BtnA.wasHold()) {
        static Channel ch{};
        ch = (ch == Channel::One) ? Channel::Two : Channel::One;
        if (unit.writeCurrentChannel(ch)) {
            M5.Speaker.tone(3000, 20);
            m5::utility::delay(50);
            M5.Speaker.tone(2000, 20);
            M5.Log.printf("Change ch:%u\n", (uint8_t)ch + 1);
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
    constexpr uint32_t FEED_INTERVAL_MS   = 2000;
    constexpr TickType_t FEED_SLEEP_TICKS = pdMS_TO_TICKS(5);
    static uint32_t s_next_feed_ms        = 0;
    const uint32_t now_ms                 = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    if (now_ms >= s_next_feed_ms) {
        s_next_feed_ms = now_ms + FEED_INTERVAL_MS;
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
