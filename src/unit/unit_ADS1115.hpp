/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1115.hpp
  @brief ADS1115 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_ADS1115_HPP
#define M5_UNIT_METER_UNIT_ADS1115_HPP

#include "unit_ADS111x.hpp"
#include "unit_EEPROM.hpp"

namespace m5 {
namespace unit {
/*!
  @class  UnitADS1115
  @brief ADS1115 unit
 */
class UnitADS1115 : public UnitADS111x {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS1115, 0x00);

   public:
    explicit UnitADS1115(const uint8_t addr = DEFAULT_ADDRESS) : UnitADS111x(addr) {
    }
    virtual ~UnitADS1115() {
    }

    ///@name Configration
    ///@{
    /*! @brief Write the input multiplexer */
    virtual bool writeMultiplexer(const ads111x::Mux mux) override {
        return write_multiplexer(mux);
    }
    //! @brief Write the programmable gain amplifier
    virtual bool writeGain(const ads111x::Gain gain) override {
        return write_gain(gain);
    }
    //! @brief Write the comparator mode
    virtual bool writeComparatorMode(const bool b) override {
        return write_comparator_mode(b);
    }
    //! @brief Write the comparator polarity
    virtual bool writeComparatorPolarity(const bool b) override {
        return write_comparator_polarity(b);
    }
    //! @brief Write the latching comparator
    virtual bool writeLatchingComparator(const bool b) override {
        return write_latching_comparator(b);
    }
    //! @brief Write the comparator queue
    virtual bool writeComparatorQueue(const ads111x::ComparatorQueue c) override {
        return write_comparator_queue(c);
    }
    ///@}

   protected:
    virtual bool on_begin() override;
};

/*!
  @class UnitAVmeterBase
  @brief ADS1115 with EEPROM
 */
class UnitAVmeterBase : public UnitADS1115 {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitAVmeterBase, 0x00);

   public:
    explicit UnitAVmeterBase(const uint8_t addr = DEFAULT_ADDRESS, const uint8_t eepromAddr = 0x00);
    virtual ~UnitAVmeterBase() {
    }

    inline float calibrationFactor() const {
        return _calibrationFactor;
    }

    virtual bool writeGain(const ads111x::Gain gain) override;

   protected:
    virtual Adapter* duplicate_adapter(const uint8_t ch) override;
    virtual bool on_begin() override;
    void apply_calibration(const ads111x::Gain gain);
    bool validChild() const {
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
