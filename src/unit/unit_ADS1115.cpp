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

namespace m5 {
namespace unit {
// class UnitADS1115
const char UnitADS1115::name[] = "UnitADS1115";
const types::uid_t UnitADS1115::uid{"UnitADS1115"_mmh3};
const types::attr_t UnitADS1115::attr{attribute::AccessI2C};

bool UnitADS1115::start_periodic_measurement(const ads111x::Sampling rate, const ads111x::Mux mux,
                                             const ads111x::Gain gain, const ads111x::ComparatorQueue comp_que)
{
    return writeSamplingRate(_cfg.rate) && writeMultiplexer(_cfg.mux) && writeGain(_cfg.gain) &&
           writeComparatorQueue(_cfg.comp_que) && UnitADS111x::start_periodic_measurement();
}

}  // namespace unit
}  // namespace m5
