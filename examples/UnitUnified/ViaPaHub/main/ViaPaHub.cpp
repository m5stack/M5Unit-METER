/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using the meter units via UnitPaHub (PaHub2)

  Device ---> PaHub2 (0x70)
                |- ch:0 UnitVmeter
                |- ch:1 UnitAmeter
                |- ch:2 UnitKmeterISO
                `- ch:3 UnitINA226-10A

  Each PaHub channel is an independent I2C bus, so units with the same address can also be connected.
  For example, to use a second UnitVmeter, declare another instance and add it to a free channel
  (e.g. hub.add(vmeter2, 4)).
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedHUB.h>
#include <M5UnitUnifiedMETER.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

// The default address of PaHub2 is 0x70 (changeable with the A0-A2 pins (v2.0) or the DIP switch (v2.1))
// https://docs.m5stack.com/en/unit/pahub2
m5::unit::UnitPaHub2 hub{};

m5::unit::UnitVmeter vmeter{};
m5::unit::UnitAmeter ameter{};
m5::unit::UnitKmeterISO kmeter_iso{};
m5::unit::UnitINA226_10A ina226{};

// PaHub channels used by each unit
constexpr uint8_t CH_VMETER{0};
constexpr uint8_t CH_AMETER{1};
constexpr uint8_t CH_KMETER_ISO{2};
constexpr uint8_t CH_INA226{3};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    if (!hub.add(vmeter, CH_VMETER) || !hub.add(ameter, CH_AMETER) || !hub.add(kmeter_iso, CH_KMETER_ISO) ||
        !hub.add(ina226, CH_INA226)) {
        M5_LOGE("Failed to add");
        m5::unit::wiring::failStop();
    }

    // The hub is the I2C device on the port
    // NessoN1 -> SoftwareI2C (M5HAL), NanoC6 / NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, hub, 400 * 1000U) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
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

    if (vmeter.updated()) {
        M5.Log.printf(">Voltage:%f\n", vmeter.voltage());
    }
    if (ameter.updated()) {
        M5.Log.printf(">Current:%f\n", ameter.current());
    }
    if (kmeter_iso.updated()) {
        M5.Log.printf(">Temperature:%f\n", kmeter_iso.temperature());
    }
    if (ina226.updated()) {
        M5.Log.printf(">INA226_A:%f\n>INA226_BV:%f\n", ina226.current(), ina226.voltage());
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
