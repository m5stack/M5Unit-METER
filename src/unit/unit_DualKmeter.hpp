/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_DualKmeter.hpp
  @brief DualKmeter Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_DUAL_KMETER_HPP
#define M5_UNIT_METER_UNIT_DUAL_KMETER_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN
#include <array>

namespace m5 {
namespace unit {

/*!
  @namespace dual_kmeter
  @brief For DualKmeter
 */
namespace dual_kmeter {

/*!
  @enum Channel
  @brief Channel to be measured
 */
enum class Channel : uint8_t {
    One,  //!< Channel 1
    Two,  //!< Channel 2
};

/*!
  @enum MeasurementUnit
  @brief measurement unit on periodic measurement
 */
enum class MeasurementUnit : uint8_t {
    Celsius,     //!< Temperature unit is celsius
    Fahrenheit,  //!< Temperature unit is fahrenheit
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 4> raw{};  //!< Raw data
    Channel channel{};             //!< Which channel?

    //@note Unit depends on setting
    inline float temperature() const
    {
        return static_cast<int32_t>(((uint32_t)raw[3] << 24) | ((uint32_t)raw[2] << 16) | ((uint32_t)raw[1] << 8) |
                                    ((uint32_t)raw[0] << 0)) *
               0.01f;
    }
};

}  // namespace dual_kmeter

/*!
  @class m5::unit::UnitDualKmeter
  @brief Module 13.2 DualKmeter unit
  @note Address changeable by DIP switch on the unit
  |Address|0|1|2|3|Note|
  |---|---|---|---|---|---|
  |0x11|OFF|OFF|OFF|OFF| as default|
  |0x12|ON|OFF|OFF|OFF||
  |0x13|OFF|ON|OFF|OFF||
  |0x14|ON|ON|OFF|OFF||
  |0x15|OFF|OFF|ON|OFF||
  |0x16|ON|OFF|ON|OFF||
  |0x17|OFF|ON|ON|OFF||
  |0x18|ON|ON|ON|OFF||
  |0x19|OFF|OFF|OFF|ON||
  |0x1A|ON|OFF|OFF|ON||
  |0x1B|OFF|ON|OFF|ON||
  |0x1C|ON|ON|OFF|ON||
  |0x1D|OFF|OFF|ON|ON||
  |0x1E|ON|OFF|ON|ON||
  |0x1F|OFF|ON|ON|ON||
  |0x20|ON|ON|ON|ON||
  @code m5::unit::DualKmeter unit{0x1A}; // Configured address
  @endcode
*/
class UnitDualKmeter : public Component, public PeriodicMeasurementAdapter<UnitDualKmeter, dual_kmeter::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitDualKmeter, 0x11);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! periodic interval(ms) if start on begin
        uint32_t interval{100};
        //! //!< measurement channel if start on begin
        dual_kmeter::Channel measurement_channel{dual_kmeter::Channel::One};
        //! //!< measurement unit if start on begin
        dual_kmeter::MeasurementUnit measurement_unit{dual_kmeter::MeasurementUnit::Celsius};
    };

    explicit UnitDualKmeter(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<dual_kmeter::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 100 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitDualKmeter()
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
    /*! Gets the measurement unit on periodic measurement */
    dual_kmeter::MeasurementUnit measurementUnit() const
    {
        return _munit;
    }
    /*! Gets the measurement channel on periodic measurement */
    dual_kmeter::Channel measurementChannel() const
    {
        return _channel;
    }
    /*! Set the measurement unit on periodic measurement */
    void setMeasurementUnit(const dual_kmeter::MeasurementUnit munit)
    {
        _munit = munit;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest temperature
    inline float temperature() const
    {
        return !empty() ? oldest().temperature() : std::numeric_limits<float>::quiet_NaN();
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
        return PeriodicMeasurementAdapter<UnitDualKmeter, dual_kmeter::Data>::startPeriodicMeasurement();
    }
    /*!
      @brief Start periodic measurement
      @oaram channel Channel to be measured
      @param interval Periodic interval(ms)
      @param munit Measurement unit
      @return True if successful
    */
    inline bool startPeriodicMeasurement(
        const uint32_t interval, const dual_kmeter::Channel channel = dual_kmeter::Channel::One,
        const dual_kmeter::MeasurementUnit munit = dual_kmeter::MeasurementUnit::Celsius)
    {
        return PeriodicMeasurementAdapter<UnitDualKmeter, dual_kmeter::Data>::startPeriodicMeasurement(interval,
                                                                                                       channel, munit);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitDualKmeter, dual_kmeter::Data>::stopPeriodicMeasurement();
    }
    ///@}

    /*!
      @brief Read status
      @param[out] status Status
      @return True if successful
    */
    bool readStatus(uint8_t& status);

    /*!
      @brief Read firmware version
      @param[out] ver version
      @return True if successful
    */
    bool readFirmwareVersion(uint8_t& ver);

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measure temperature single shot
      @param[out] data Measuerd data
      @param channel Channel to be measured
      @param munit  measurement unit
      @param timeoutMs Measurement timeout time(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshot(dual_kmeter::Data& d, const dual_kmeter::Channel channel,
                           dual_kmeter::MeasurementUnit munit = dual_kmeter::MeasurementUnit::Celsius,
                           const uint32_t timeoutMs           = 100);
    /*!
      @brief Measure internal temperature single shot
      @param[out] data Measuerd data
      @param munit  measurement unit
      @param timeoutMs Measurement timeout time(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool measureInternalSingleshot(dual_kmeter::Data& d, const dual_kmeter::Channel channel,
                                   const dual_kmeter::MeasurementUnit munit = dual_kmeter::MeasurementUnit::Celsius,
                                   const uint32_t timeoutMs                 = 100);
    ///@}

    ///@name Channel
    ///@{
    /*!
      @brief Read the current measure channel
      @param[out] channel Channel
      @return True if successful
     */
    bool readCurrentChannel(dual_kmeter::Channel& channel);
    /*!
      @brief Write the current measure channel
      @param channel Channel
      @return True if successful
     */
    bool writeCurrentChannel(const dual_kmeter::Channel channel);
    ///@}

protected:
    inline bool read_register(const uint8_t reg, uint8_t* rbuf, const uint32_t len)
    {
        return readRegister(reg, rbuf, len, 0, false);
    }
    inline bool read_register8(const uint8_t reg, uint8_t& v)
    {
        return readRegister8(reg, v, 0, false);
    }

    bool start_periodic_measurement();
    bool start_periodic_measurement(const uint32_t interval, const dual_kmeter::Channel channel,
                                    const dual_kmeter::MeasurementUnit munit);
    bool stop_periodic_measurement();

    bool read_measurement(dual_kmeter::Data& d, const dual_kmeter::MeasurementUnit munit);
    bool read_internal_measurement(dual_kmeter::Data& d, const dual_kmeter::MeasurementUnit munit);

    bool is_data_ready()
    {
        uint8_t s{};
        return readStatus(s) && (s == 0U);
    }
    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitDualKmeter, dual_kmeter::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<dual_kmeter::Data>> _data{};
    dual_kmeter::MeasurementUnit _munit{dual_kmeter::MeasurementUnit::Celsius};
    dual_kmeter::Channel _channel{}, _current_channel{};
    config_t _cfg{};
};

namespace dual_kmeter {
///@cond
namespace command {
constexpr uint8_t TEMPERATURE_CELSIUS_REG{0x00};                    // R
constexpr uint8_t TEMPERATURE_FAHRENHEIT_REG{0x04};                 // R
constexpr uint8_t INTERNAL_TEMPERATURE_CELSIUS_REG{0x10};           // R
constexpr uint8_t INTERNAL_TEMPERATURE_FAHRENHEIT_REG{0x14};        // R
constexpr uint8_t CHANNEL_REG{0x20};                                // R/W
constexpr uint8_t STATUS_REG{0x30};                                 // R
constexpr uint8_t TEMPERATURE_CELSIUS_STRING_REG{0x40};             // R
constexpr uint8_t TEMPERATURE_FAHRENHEITSTRING_REG{0x50};           // R
constexpr uint8_t INTERNAL_TEMPERATURE_CELSIUS_STRING_REG{0x60};    // R
constexpr uint8_t INTERNAL_TEMPERATURE_FAHRENHEITSTRING_REG{0x70};  // R
constexpr uint8_t FIRMWARE_VERSION_REG{0xFE};                       // R
}  // namespace command
///@endcond
}  // namespace dual_kmeter

}  // namespace unit
}  // namespace m5

#endif
