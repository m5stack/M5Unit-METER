/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_av_base.hpp
  @brief A/Vmeter base class for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_AV_BASE_HPP
#define M5_UNIT_METER_UNIT_AV_BASE_HPP

#include "unit_ADS1115.hpp"
#include "unit_EEPROM.hpp"

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitAVmeterBase
  @brief Base class for A/VMeter
 */
class UnitAVmeterBase : public UnitADS1115 {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitAVmeterBase, 0x00);

public:
    explicit UnitAVmeterBase(const uint8_t addr = DEFAULT_ADDRESS, const uint8_t eepromAddr = 0x00);
    virtual ~UnitAVmeterBase()
    {
    }

    virtual bool begin() override;

    inline float calibrationFactor() const
    {
        return _calibrationFactor;
    }

    virtual bool writeGain(const ads111x::Gain gain) override;

protected:
    std::shared_ptr<Adapter> ensure_adapter(const uint8_t ch);
    void apply_calibration(const ads111x::Gain gain);
    bool validChild() const
    {
        return _valid;
    }

protected:
    m5::unit::meter::UnitEEPROM _eeprom{};

private:
    float _calibrationFactor{1.0f};
    bool _valid{};  // Did the constructor correctly add the child unit?
};

}  // namespace unit
}  // namespace m5
#endif
