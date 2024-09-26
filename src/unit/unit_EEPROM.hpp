/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_EEPROM.hpp
  @brief Accessor unit to EEPROM that holds calibration information
*/
#ifndef M5_UNIT_METER_UNIT_EEPROM_HPP
#define M5_UNIT_METER_UNIT_EEPROM_HPP

#include "unit_ADS111x.hpp"
#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>
#include <array>

namespace m5 {
namespace unit {
/*!
  @namespace meter
  @brief namespace for Meter
 */
namespace meter {

/*!
  @class UnitEEPROM
  @brief Accessor unit to EEPROM that holds calibration data
  @note Ameter/Vmeter has the EEPROM for holding calibration data
 */
class UnitEEPROM : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitEEPROM, 0x00);

public:
    explicit UnitEEPROM(const uint8_t addr = DEFAULT_ADDRESS) : Component(addr)
    {
    }
    virtual ~UnitEEPROM()
    {
    }

    inline int16_t hope(m5::unit::ads111x::Gain gain) const
    {
        return _calibration[m5::stl::to_underlying(gain)].hope;
    }
    inline int16_t actual(m5::unit::ads111x::Gain gain) const
    {
        return _calibration[m5::stl::to_underlying(gain)].actual;
    }
    inline float calibrationFactor(m5::unit::ads111x::Gain gain) const
    {
        return actual(gain) ? (float)hope(gain) / actual(gain) : 1.0f;
    }

    bool readCalibration();

protected:
    bool read_calibration(const m5::unit::ads111x::Gain gain, int16_t& hope, int16_t& actual);

private:
    struct Calibration {
        int16_t hope{1};
        int16_t actual{1};
    };
    std::array<Calibration, 8 /*Gain*/> _calibration{};
};

}  // namespace meter
}  // namespace unit
}  // namespace m5
#endif
