/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS111x.hpp
  @brief Base class for ADS111x families
*/
#ifndef M5_UNIT_METER_UNIT_ADS111X_HPP
#define M5_UNIT_METER_UNIT_ADS111X_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>

namespace m5 {
namespace unit {

/*!
  @namespace ads111x
  @brief For ADS111x families
 */
namespace ads111x {
/*!
  @enum Mux
  @brief Input multiplexer
  @warning This feature serve nofunction on the ADS1113 and ADS1114
*/
enum class Mux : uint8_t {
    AIN_01,  //!< Positive:AIN0 Negative:AIN1 as default
    AIN_03,  //!< Positive:AIN0 Negative:AIN3
    AIN_13,  //!< Positive:AIN1 Negative:AIN3
    AIN_23,  //!< Positive:AIN2 Negative:AIN3
    GND_0,   //!< Positive:AIN0 Negative:GND
    GND_1,   //!< Positive:AIN1 Negative:GND
    GND_2,   //!< Positive:AIN2 Negative:GND
    GND_3,   //!< Positive:AIN3 Negative:GND
};

/*!
  @enum Gain
  @brief Programmable gain amplifier
  @warning This feature serve nofunction on the ADS1113
 */
enum class Gain : uint8_t {
    PGA_6144,  //!< +/- 6.144 V
    PGA_4096,  //!< +/- 4.096 V
    PGA_2048,  //!< +/- 2.048 V as default
    PGA_1024,  //!< +/- 1.024 V
    PGA_512,   //!< +/- 0.512 V
    PGA_256,   //!< +/- 0.256 V
    // [6,7] PGA_256 (duplicate)
};

/*!
  @enum Sampling
  @brief Data rate setting (samples per second)
 */
enum class Sampling : uint8_t {
    Rate8,    //!< 8 sps
    Rate16,   //!< 16 sps
    Rate32,   //!< 32 sps
    Rate64,   //!< 64 sps
    Rate128,  //!< 128 sps as default
    Rate250,  //!< 250 sps
    Rate475,  //!< 475 sps
    Rate860,  //!< 860 sps
};

/*!
  @enum ComparatorQueue
  @brief the value determines the number of successive conversions exceeding the upper orlower threshold required
  @warning This feature serve nofunction on the ADS1113
*/
enum class ComparatorQueue : uint8_t {
    One,      //!< Assert after one conversion
    Two,      //!< Assert after two conversion
    Four,     //!< Assert after four conversion
    Disable,  //!< Disable comparator and set ALERT/RDY pin to high-impedance as
              //!< default
};

///@cond
struct Config {
    inline bool os() const
    {
        return value & (1U << 15);
    }
    inline Mux mux() const
    {
        return static_cast<Mux>((value >> 12) & 0x07);
    }
    inline Gain pga() const
    {
        return static_cast<Gain>((value >> 9) & 0x07);
    }
    inline bool mode() const
    {
        return value & (1U << 8);
    }
    inline Sampling dr() const
    {
        return static_cast<Sampling>((value >> 5) & 0x07);
    }
    inline bool comp_mode() const
    {
        return value & (1U << 4);
    }
    inline bool comp_pol() const
    {
        return value & (1U << 3);
    }
    inline bool comp_lat() const
    {
        return value & (1U << 2);
    }
    inline ComparatorQueue comp_que() const
    {
        return static_cast<ComparatorQueue>(value & 0x03);
    }
    inline void os(const bool b)
    {
        value = (value & ~(1U << 15)) | ((b ? 1U : 0) << 15);
    }
    inline void mux(const Mux m)
    {
        value = (value & ~(0x07 << 12)) | ((m5::stl::to_underlying(m) & 0x07) << 12);
    }
    inline void pga(const Gain g)
    {
        value = (value & ~(0x07 << 9)) | ((m5::stl::to_underlying(g) & 0x07) << 9);
    }
    inline void mode(const bool b)
    {
        value = (value & ~(1U << 8)) | ((b ? 1U : 0) << 8);
    }
    inline void dr(const Sampling r)
    {
        value = (value & ~(0x07 << 5)) | ((m5::stl::to_underlying(r) & 0x07) << 5);
    }
    inline void comp_mode(const bool b)
    {
        value = (value & ~(1U << 4)) | ((b ? 1U : 0) << 4);
    }
    inline void comp_pol(const bool b)
    {
        value = (value & ~(1U << 3)) | ((b ? 1U : 0) << 3);
    }
    inline void comp_lat(const bool b)
    {
        value = (value & ~(1U << 2)) | ((b ? 1U : 0) << 2);
    }
    inline void comp_que(const ComparatorQueue c)
    {
        value = (value & ~0x03U) | (m5::stl::to_underlying(c) & 0x03);
    }
    uint16_t value{};
};
///@endcond

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    uint16_t raw{};
    //! @brief ADC
    inline int16_t adc() const
    {
        return static_cast<int16_t>(raw);
    }
};

}  // namespace ads111x

/*!
  @class m5::unit::UnitADS111x
  @brief Base class for ADS111x series
 */
class UnitADS111x : public Component, public PeriodicMeasurementAdapter<UnitADS111x, ads111x::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS111x, 0x00);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //!  sampling rate if start on begin
        ads111x::Sampling rate{ads111x::Sampling::Rate128};
        //! Mux if start on begin (Not supported in some classes)
        ads111x::Mux mux{ads111x::Mux::AIN_01};
        //! Gain if start on begin (Not supported in some classes)
        ads111x::Gain gain{ads111x::Gain::PGA_2048};
        //! ComparatorQueue if start on begin (Not supported in some classes)
        ads111x::ComparatorQueue comp_que{ads111x::ComparatorQueue::Disable};
    };

    explicit UnitADS111x(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<ads111x::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitADS111x()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Properties
    ///@{
    /*!
      @brief Coefficient value
      @note Changes as gain changes
    */
    inline float coefficient() const
    {
        return _coefficient;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured ADC
    inline int16_t adc() const
    {
        return !empty() ? oldest().adc() : std::numeric_limits<int16_t>::min();
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement in the current settings
      @return True if successful
    */
    inline bool startPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitADS111x, ads111x::Data>::startPeriodicMeasurement();
    }
    /*!
      @brief Start periodic measurement
      @param rate Sampling rate
      @param mux Input multiplexer (Not supported in some classes)
      @param gain Programmable gain amplifier (Not supported in some classes)
      @param comp_que Comparator queue (Not supported in some classes)
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const ads111x::Sampling rate, const ads111x::Mux mux, const ads111x::Gain gain,
                                         const ads111x::ComparatorQueue comp_que)
    {
        return PeriodicMeasurementAdapter<UnitADS111x, ads111x::Data>::startPeriodicMeasurement(rate, mux, gain,
                                                                                                comp_que);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
     */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitADS111x, ads111x::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@warning ADS1113, ADS1114 and ADS1115 differ in the items that can be set
    ///@name Configration
    ///@{
    /*! @brief Gets the input multiplexer */
    inline ads111x::Mux multiplexer() const
    {
        return _ads_cfg.mux();
    }
    //! @brief Gets the programmable gain amplifier
    ads111x::Gain gain() const;
    //! @brief Gets the sampling rate
    inline ads111x::Sampling samplingRate() const
    {
        return _ads_cfg.dr();
    }
    /*!
      @brief Gets the comparator mode
      @retval true Window comparator
      @retval false Traditional comparator
     */
    inline bool comparatorMode() const
    {
        return _ads_cfg.comp_mode();
    }
    /*!
      @brief Gets the comparator polarity
      @retval true Active high
      @retval false Active low
     */
    inline bool comparatorPolarity() const
    {
        return _ads_cfg.comp_pol();
    }
    /*!
      @brief Gets the Latching comparator
      @retval true Latching comparator
      @retval false Nonlatching comparator
    */
    inline bool latchingComparator() const
    {
        return _ads_cfg.comp_lat();
    }
    //! @brief Gets the comparator queue
    inline ads111x::ComparatorQueue comparatorQueue() const
    {
        return _ads_cfg.comp_que();
    }

    //! @brief Write the input multiplexer
    virtual bool writeMultiplexer(const ads111x::Mux mux) = 0;
    /*!
      @brief Write the programmable gain amplifier
      @warning the threshould values  must be updated whenever the PGA settings are changed
      @sa writeThreshold
     */
    virtual bool writeGain(const ads111x::Gain gain) = 0;
    /*! @brief Write the data rate  */
    bool writeSamplingRate(const ads111x::Sampling rate);
    //! @brief Write the comparator mode
    virtual bool writeComparatorMode(const bool b) = 0;
    //! @brief Write the comparator polarity
    virtual bool writeComparatorPolarity(const bool b) = 0;
    //! @brief Write the latching comparator
    virtual bool writeLatchingComparator(const bool b) = 0;
    //! @brief Write the comparator queue
    virtual bool writeComparatorQueue(const ads111x::ComparatorQueue c) = 0;
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @details Measuring in the current settings
      @param[out] data Measuerd data
      @param timeoutMillis Timeout for measure
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Until it can be measured, it will be blocked until the timeout
      time
    */
    bool measureSingleshot(ads111x::Data& d, const uint32_t timeoutMillis = 1000U);
    ///@}

    ///@name Threshold
    ///@{
    /*!
      @brief Reads the threshold values
      @param[out] high upper threshold value
      @param[out] low lower threshold value
      @return True if successful
    */
    bool readThreshold(int16_t& high, int16_t& low);
    /*!
      @brief Write the threshold values
      @param high upper threshold value
      @param low lower threshold value
      @return True if successful
      @warning The high value must always be greater than the low value
    */
    bool writeThreshold(const int16_t high, const int16_t low);
    ///@}

    /*!
      @brief General reset
      @details Reset using I2C general call
      @warning This is a reset by General command, the command is also sent to all devices with I2C connections
     */
    bool generalReset();

protected:
    bool start_periodic_measurement();
    virtual bool start_periodic_measurement(const ads111x::Sampling rate, const ads111x::Mux mux,
                                            const ads111x::Gain gain, const ads111x::ComparatorQueue comp_que) = 0;
    bool stop_periodic_measurement();

    bool read_adc_raw(ads111x::Data& d);
    bool start_single_measurement();
    bool in_conversion();

    bool read_config(ads111x::Config& c);
    bool write_config(const ads111x::Config& c);
    void apply_interval(const ads111x::Sampling rate);
    virtual void apply_coefficient(const ads111x::Gain gain);

    bool write_multiplexer(const ads111x::Mux mux);
    bool write_gain(const ads111x::Gain gain);
    bool write_comparator_mode(const bool b);
    bool write_comparator_polarity(const bool b);
    bool write_latching_comparator(const bool b);
    bool write_comparator_queue(const ads111x::ComparatorQueue c);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitADS111x, ads111x::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<ads111x::Data>> _data{};
    float _coefficient{};
    ads111x::Config _ads_cfg{};
    config_t _cfg{};
};

///@cond
namespace ads111x {
namespace command {

constexpr uint8_t CONVERSION_REG{0x00};
constexpr uint8_t CONFIG_REG{0x01};
constexpr uint8_t LOW_THRESHOLD_REG{0x02};
constexpr uint8_t HIGH_THRESHOLD_REG{0x03};

}  // namespace command
}  // namespace ads111x
///@endcond

}  // namespace unit
}  // namespace m5

#endif
