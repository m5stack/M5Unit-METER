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
#include <Wire.h>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_INA226_1A) && !defined(USING_UNIT_INA226_10A)
// #define USING_UNIT_INA226_1A
// #define USING_UNIT_INA226_10A
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
#else
#error "Choose unit"
#endif
}  // namespace

void setup()
{
    M5.begin();

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    lcd.setFont(&fonts::AsciiFont8x16);

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    using namespace m5::unit::ina226;

    M5.update();
    auto touch = M5.Touch.getDetail();
    Units.update();

    if (unit.updated()) {
        M5.Log.printf(">A:%f\n>SV:%f\n>BV:%f\n>W:%f\n", unit.current(), unit.shuntVoltage(), unit.voltage(),
                      unit.power());

        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), 16 * 4);
        lcd.setCursor(0, 0);
        lcd.printf(
            " C:%5.2f mA\n"
            "SV:%5.2f mV\n"
            "BV:%5.2f mV\n"
            " P:%5.2f mW",
            unit.current(), unit.shuntVoltage(), unit.voltage(), unit.power());
        lcd.endWrite();
    }

    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        static bool single{};
        single = !single;
        if (single) {
            M5.Speaker.tone(1500, 20);
            Data d{};
            unit.stopPeriodicMeasurement();
            if (unit.measureSingleshot(d)) {
                M5.Log.printf("Single:A:%f SV:%f BV:%f W:%f\n", unit.current(), unit.shuntVoltage(), unit.voltage(),
                              unit.power());
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
