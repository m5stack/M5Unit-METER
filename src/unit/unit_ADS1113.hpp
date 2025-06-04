/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1113.hpp
  @brief ADS1113 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_ADS1113_HPP
#define M5_UNIT_METER_UNIT_ADS1113_HPP

#include "unit_ADS111x.hpp"

namespace m5 {
namespace unit {
/*!
  @class  m5::unit::UnitADS1113
  @brief ADS1113 unit
 */
class UnitADS1113 : public UnitADS111x {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS1113, 0x00);

public:
    explicit UnitADS1113(const uint8_t addr = DEFAULT_ADDRESS) : UnitADS111x(addr)
    {
    }
    virtual ~UnitADS1113()
    {
    }

    ///@name Configration
    ///@{
    /*! @brief Not support @warning Not support */
    virtual bool writeMultiplexer(const ads111x::Mux) override
    {
        return false;
    }
    //!  @brief Not support @warning Not support
    virtual bool writeGain(const ads111x::Gain) override
    {
        return false;
    }
    //!  @brief Not support @warning Not support
    virtual bool writeComparatorMode(const bool) override
    {
        return false;
    }
    //!  @brief Not support @warning Not support
    virtual bool writeComparatorPolarity(const bool) override
    {
        return false;
    }
    //!  @brief Not support @warning Not support
    virtual bool writeLatchingComparator(const bool) override
    {
        return false;
    }
    //!  @brief Not support @warning Not support
    virtual bool writeComparatorQueue(const ads111x::ComparatorQueue) override
    {
        return false;
    }
    ///@}

protected:
    virtual bool start_periodic_measurement(const ads111x::Sampling rate, const ads111x::Mux mux,
                                            const ads111x::Gain gain, const ads111x::ComparatorQueue comp_que) override;
};
}  // namespace unit
}  // namespace m5
#endif
