/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1114.hpp
  @brief ADS1114 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_ADS1114_HPP
#define M5_UNIT_METER_UNIT_ADS1114_HPP

#include "unit_ADS111x.hpp"

namespace m5 {
namespace unit {
/*!
  @class  UnitADS1114
  @brief ADS1114 unit
 */
class UnitADS1114 : public UnitADS111x {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS1114, 0x00);

public:
    explicit UnitADS1114(const uint8_t addr = DEFAULT_ADDRESS) : UnitADS111x(addr)
    {
    }
    virtual ~UnitADS1114()
    {
    }

    ///@name Configration
    ///@{
    /*!  @brief Not support @warning Not support */
    virtual bool writeMultiplexer(const ads111x::Mux) override
    {
        return false;
    }
    //! @brief Write the programmable gain amplifier
    virtual bool writeGain(const ads111x::Gain gain) override
    {
        return write_gain(gain);
    }
    //! @brief Write the comparator mode
    virtual bool writeComparatorMode(const bool b) override
    {
        return write_comparator_mode(b);
    }
    //! @brief Write the comparator polarity
    virtual bool writeComparatorPolarity(const bool b) override
    {
        return write_comparator_polarity(b);
    }
    //! @brief Write the latching comparator
    virtual bool writeLatchingComparator(const bool b) override
    {
        return write_latching_comparator(b);
    }
    //! @brief Write the comparator queue
    virtual bool writeComparatorQueue(const ads111x::ComparatorQueue c) override
    {
        return write_comparator_queue(c);
    }
    ///@}

protected:
    virtual bool start_periodic_measurement(const ads111x::Sampling rate, const ads111x::Mux mux,
                                            const ads111x::Gain gain, const ads111x::ComparatorQueue comp_que) override;
};
}  // namespace unit
}  // namespace m5
#endif
