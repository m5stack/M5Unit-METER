/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_av_base.cpp
  @brief A/Vmeter base class for M5UnitUnified
*/
#include "unit_av_base.hpp"

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::ads111x;
using namespace m5::unit::ads111x::command;

namespace m5 {
namespace unit {

// class UnitAVmeterBase
const char UnitAVmeterBase::name[] = "UnitAVmeterBase";
const types::uid_t UnitAVmeterBase::uid{"UnitAVmeterBase"_mmh3};
const types::attr_t UnitAVmeterBase::attr{attribute::AccessI2C};

UnitAVmeterBase::UnitAVmeterBase(const uint8_t addr, const uint8_t eepromAddr) : UnitADS1115(addr), _eeprom(eepromAddr)
{
    // Form a parent-child relationship
    auto cfg         = component_config();
    cfg.max_children = 1;
    component_config(cfg);
    _valid = add(_eeprom, 0) && m5::utility::isValidI2CAddress(_eeprom.address());
}

bool UnitAVmeterBase::begin()
{
    if (!validChild()) {
        M5_LIB_LOGE("Child unit is invalid %x", _eeprom.address());
        return false;
    }
    if (!_eeprom.readCalibration()) {
        return false;
    }
    apply_calibration(_ads_cfg.pga());

    return UnitADS111x::begin();
}

bool UnitAVmeterBase::writeGain(const ads111x::Gain gain)
{
    if (UnitADS1115::writeGain(gain)) {
        apply_calibration(gain);
        return true;
    }
    return false;
}

std::shared_ptr<Adapter> UnitAVmeterBase::ensure_adapter(const uint8_t ch)
{
    if (ch > 0) {
        M5_LIB_LOGE("Invalid channel %u", ch);
        return std::make_shared<Adapter>();  // Empty adapter
    }
    auto unit = child(ch);
    if (!unit) {
        M5_LIB_LOGE("Not exists unit %u", ch);
        return std::make_shared<Adapter>();  // Empty adapter
    }
    auto ad = asAdapter<AdapterI2C>(Adapter::Type::I2C);
    return ad ? std::shared_ptr<Adapter>(ad->duplicate(unit->address())) : std::make_shared<Adapter>();
}

void UnitAVmeterBase::apply_calibration(const Gain gain)
{
    _calibrationFactor = _eeprom.calibrationFactor(gain);
}

}  // namespace unit
}  // namespace m5
