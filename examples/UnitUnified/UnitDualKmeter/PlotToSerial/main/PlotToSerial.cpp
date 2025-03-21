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

namespace {
m5::unit::UnitUnified Units;
m5::unit::UnitDualKmeter unit{0x11};  // Configured address
auto& lcd = M5.Display;
}  // namespace

using namespace m5::unit::dual_kmeter;

void setup()
{
    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::in_i2c_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::in_i2c_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 100000U);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    Units.update();

    if (unit.updated()) {
        auto d = unit.oldest();
        M5.Log.printf(">Temperature%d:%.2f\n", (int)d.channel + 1, d.temperature());
    }

    // Togle single <-> periodic
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
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
                M5.Log.printf("Single:interbal temperature %.2f\n", d.temperature());
            }
            lcd.clear(TFT_BLUE);
        } else {
            M5.Speaker.tone(3000, 40);
            unit.startPeriodicMeasurement();
            lcd.clear(TFT_DARKGREEN);
        }
    }

    // Togle channel 1 <-> 2
    if (M5.BtnA.wasHold() || touch.wasHold()) {
        static Channel ch{};
        ch = (ch == Channel::One) ? Channel::Two : Channel::One;
        if (unit.writeCurrentChannel(ch)) {
            M5.Speaker.tone(3000, 20);
            m5::utility::delay(50);
            M5.Speaker.tone(2000, 20);
            M5.Log.printf("Change ch:%u\n", (int)ch + 1);
        }
    }
}
