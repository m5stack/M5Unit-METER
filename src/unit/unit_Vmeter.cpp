/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Vmeter.cpp
  @brief Vmeter (ADS1115 + CA-IS3020S) Unit for M5UnitUnified
*/
#include "unit_Vmeter.hpp"
#include <M5Utility.hpp>
namespace m5 {
namespace unit {

using namespace m5::utility::mmh3;
using namespace ads111x;
using namespace ads111x::command;
using namespace m5::unit::types;

const char UnitVmeter::name[] = "UnitVmeter";
const types::uid_t UnitVmeter::uid{"UnitVmeter"_mmh3};
const types::attr_t UnitVmeter::attr{attribute::AccessI2C};

void UnitVmeter::apply_coefficient(const ads111x::Gain gain)
{
    UnitADS111x::apply_coefficient(gain);
    _correction = resolution() * _eeprom.calibrationFactor(gain);
}

}  // namespace unit
}  // namespace m5
