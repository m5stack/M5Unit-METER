/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Vmeter.hpp
  @brief Vmeter (ADS1115 + CA-IS3020S) Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_V_METER_HPP
#define M5_UNIT_METER_UNIT_V_METER_HPP

#include "unit_ADS1115.hpp"
#include <limits>  // NaN

namespace m5 {
namespace unit {
/*!
  @class UnitVmeter
  @brief Voltmeter Unit is a voltage meter that can monitor the voltage in real time
 */
class UnitVmeter : public UnitAVmeterBase {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitVmeter, 0x49);

   public:
    constexpr static uint8_t DEFAULT_EEPROM_ADDRESS{0x53};
    constexpr static float PRESSURE_COEFFICIENT{0.01591895f};

    explicit UnitVmeter(const uint8_t addr = DEFAULT_ADDRESS, const uint8_t eepromAddr = DEFAULT_EEPROM_ADDRESS)
        : UnitAVmeterBase(addr, eepromAddr) {
        // component_config().clock is set at ADS111x constructor
    }
    virtual ~UnitVmeter() {
    }

    //! @brief Resolution of 1 LSB
    inline float resolution() const {
        return coefficient() / PRESSURE_COEFFICIENT;
    }

    //! @brief Gets the correction value
    inline float correction() const {
        return _correction;
    }

    //! @brief Oldest voltage (mV)
    inline float voltage() const {
        return !empty() ? correction() * adc() : std::numeric_limits<float>::quiet_NaN();
    }

   protected:
    virtual void apply_coefficient(const ads111x::Gain gain) override;

   private:
    float _correction{1.0f};
};
}  // namespace unit
}  // namespace m5
#endif
