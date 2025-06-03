/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1114.cpp
  @brief ADS1114 Unit for M5UnitUnified
*/
#include "unit_ADS1114.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::ads111x;
using namespace m5::unit::ads111x::command;

namespace m5 {
namespace unit {
// class UnitADS1114
const char UnitADS1114::name[] = "UnitADS1114";
const types::uid_t UnitADS1114::uid{"UnitADS1114"_mmh3};
const types::attr_t UnitADS1114::attr{attribute::AccessI2C};

bool UnitADS1114::start_periodic_measurement(const ads111x::Sampling rate, const ads111x::Mux, const ads111x::Gain gain,
                                             const ads111x::ComparatorQueue comp_que)
{
    M5_LIB_LOGW("mux is not support");
    return writeSamplingRate(_cfg.rate) && writeGain(_cfg.gain) && writeComparatorQueue(_cfg.comp_que) &&
           UnitADS111x::start_periodic_measurement();
}

}  // namespace unit
}  // namespace m5
