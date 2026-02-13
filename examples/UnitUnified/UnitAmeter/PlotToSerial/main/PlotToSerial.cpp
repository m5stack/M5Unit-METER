/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example: UnitAmeter
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
#include <M5HAL.hpp>  // For NessoN1
#include <Wire.h>
#include <cmath>

using namespace m5::unit::ads111x;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitAmeter unit;

Sampling rate{Sampling::Rate32};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // Settings
    auto cfg = unit.config();
    cfg.gain = Gain::PGA_1024;
    cfg.rate = rate;
    unit.config(cfg);

    auto board       = M5.getBoard();
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);

    // For NessoN1 GROVE
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // Port A of the NessoN1 is QWIIC, then use portB (GROVE)
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(NessoN1): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        // Wire is used internally, so SoftwareI2C handles the unit
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        if (!Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.clear(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    } else {
        // Using TwoWire
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, unit.component_config().clock);
        if (!Units.add(unit, Wire) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.clear(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    M5_LOGI(">RES:%f COEE:%f CF:%f CORR:%f periodic :%d", unit.resolution(), unit.coefficient(),
            unit.calibrationFactor(), unit.correction(), unit.inPeriodic());

    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        while (unit.available()) {
            M5.Log.printf(">Current:%f\n", unit.current());
            unit.discard();
        }
    }

    if (M5.BtnA.wasClicked()) {
        {
            rate = static_cast<Sampling>((m5::stl::to_underlying(rate) + 1) & 0x07);
            if (!unit.writeSamplingRate(rate)) {
                M5_LOGE("Failed to writeSamplingRate");
            }
        }
    }
}
