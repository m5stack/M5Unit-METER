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
#include <M5HAL.hpp>  // For NessoN1
#include <Wire.h>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_INA226_1A) && !defined(USING_UNIT_INA226_10A) && !defined(BUILTIN_UNIT_INA226_10A)
// #define USING_UNIT_INA226_1A
// #define USING_UNIT_INA226_10A
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
        while (true) {
            m5::utility::delay(10000);
        }
    }

    if (!Units.add(unit, M5.In_I2C) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

#else
    auto board       = M5.getBoard();
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);

    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Wire1 exists but is reserved for HatPort — cannot be used for GROVE.
    //   Reconfiguring Wire to GROVE pins breaks In_I2C, causing ESP_ERR_INVALID_STATE in M5.update().
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, unit.component_config().clock);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#endif

    lcd.setFont(&fonts::AsciiFont8x16);

    M5_LOGI("M5UnitUnified has been begun");
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
