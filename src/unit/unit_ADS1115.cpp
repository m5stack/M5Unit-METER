/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1115.cpp
  @brief ADS1115 Unit for M5UnitUnified
*/
#include "unit_ADS1115.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::ads111x;
using namespace m5::unit::ads111x::command;

namespace {
constexpr Gain gain_table[] = {
    Gain::PGA_6144, Gain::PGA_4096, Gain::PGA_2048, Gain::PGA_1024, Gain::PGA_512, Gain::PGA_256,
};
}

namespace m5 {
namespace unit {
// class UnitADS1115
const char UnitADS1115::name[] = "UnitADS1115";
const types::uid_t UnitADS1115::uid{"UnitADS1115"_mmh3};
const types::uid_t UnitADS1115::attr{0};
bool UnitADS1115::on_begin() {
    return writeSamplingRate(_cfg.rate) && writeMultiplexer(_cfg.mux) && writeGain(_cfg.gain) &&
           writeComparatorQueue(_cfg.comp_que);
}

// class UnitAVmeterBase
const char UnitAVmeterBase::name[] = "UnitAVmeterBase";
const types::uid_t UnitAVmeterBase::uid{"UnitAVmeterBase"_mmh3};
const types::uid_t UnitAVmeterBase::attr{0};

UnitAVmeterBase::UnitAVmeterBase(const uint8_t addr, const uint8_t eepromAddr)
    : UnitADS1115(addr), _eeprom(eepromAddr) {
    // Form a parent-child relationship
    auto cfg         = component_config();
    cfg.max_children = 1;
    component_config(cfg);
    _valid = add(_eeprom, 0) && m5::utility::isValidI2CAddress(_eeprom.address());
}

bool UnitAVmeterBase::on_begin() {
    if (!validChild()) {
        M5_LIB_LOGE("Child unit is invalid %x", _eeprom.address());
        return false;
    }
    if (!_eeprom.readCalibration()) {
        return false;
    }
    apply_calibration(_ads_cfg.pga());
    return UnitADS1115::on_begin();
}

bool UnitAVmeterBase::writeGain(const ads111x::Gain gain) {
    if (UnitADS1115::writeGain(gain)) {
        apply_calibration(gain);
        return true;
    }
    return false;
}

Adapter* UnitAVmeterBase::duplicate_adapter(const uint8_t ch) {
    if (ch != 0) {
        M5_LIB_LOGE("Invalid channel %u", ch);
        return nullptr;
    }
    auto myadapter = adapter();
    return myadapter ? myadapter->duplicate(_eeprom.address()) : nullptr;
}

void UnitAVmeterBase::apply_calibration(const Gain gain) {
    _calibrationFactor = _eeprom.calibrationFactor(gain);
}

}  // namespace unit
}  // namespace m5
