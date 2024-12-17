/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_KmeterISO.hpp
  @brief KmeterISO Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_KMETERISO_HPP
#define M5_UNIT_METER_UNIT_KMETERISO_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN
#include <array>

namespace m5 {
namespace unit {

namespace kmeterISO {

/*!
  @enum MeasurementUnit
  @brief measurement unit on periodic measurement
 */
enum MeasurementUnit : uint8_t {
    Celsius,     //!< Temperature is celsius
    Fahrenheit,  //!< Temperature is fahrenheit
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 4> raw{};  // raw data
    inline float temperature() const
    {
        return static_cast<int32_t>(((uint32_t)raw[3] << 24) | ((uint32_t)raw[2] << 16) | ((uint32_t)raw[1] << 8) |
                                    ((uint32_t)raw[0] << 0)) *
               0.01f;
    }
};
}  // namespace kmeterISO

/*!
  @class UnitKmeterISO
  @brief KMeterISO unitis an integrated K-type thermocouple sensor unit that integrates the functions of "acquisition +
  isolation + communication"
 */
class UnitKmeterISO : public Component, public PeriodicMeasurementAdapter<UnitKmeterISO, kmeterISO::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitKmeterISO, 0x66);

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
        //! //!< measurement unit if start on begin
        kmeterISO::MeasurementUnit measurement_unit{kmeterISO::MeasurementUnit::Celsius};
    };

    explicit UnitKmeterISO(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<kmeterISO::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 100 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitKmeterISO()
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
    kmeterISO::MeasurementUnit measurementUnit() const
    {
        return _munit;
    }
    /*! Set the measurement unit on periodic measurement */
    void setMeasurementUnit(const kmeterISO::MeasurementUnit munit)
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
        return PeriodicMeasurementAdapter<UnitKmeterISO, kmeterISO::Data>::startPeriodicMeasurement();
    }
    /*!
      @brief Start periodic measurement
      @param interval Periodic interval(ms)
      @param munit Measurement unit
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const uint32_t interval,
                                         const kmeterISO::MeasurementUnit munit = kmeterISO::Celsius)
    {
        return PeriodicMeasurementAdapter<UnitKmeterISO, kmeterISO::Data>::startPeriodicMeasurement(interval, munit);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitKmeterISO, kmeterISO::Data>::stopPeriodicMeasurement();
    }
    ///@}

    /*!
      @brief Read status
      @param[out] status Status
      @return True if successful
    */
    bool readStatus(uint8_t& status);
    /*!
      @brief Ready to read data?
      @return True if ready to read data
    */
    bool isReady()
    {
        uint8_t s{};
        return readStatus(s) && (s == 0U);
    }

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
      @param munit  measurement unit
      @param timeoutMs Measurement timeout time(ms)
      @return True if successful
     */
    bool measureSingleshot(kmeterISO::Data& d, kmeterISO::MeasurementUnit munit = kmeterISO::MeasurementUnit::Celsius,
                           const uint32_t timeoutMs = 0);
    /*!
      @brief Measure internal temperature single shot
      @param[out] data Measuerd data
      @param munit  measurement unit
      @param timeoutMs Measurement timeout time(ms)
      @return True if successful
     */
    bool measureInternalSingleshot(kmeterISO::Data& d,
                                   kmeterISO::MeasurementUnit munit = kmeterISO::MeasurementUnit::Celsius,
                                   const uint32_t timeoutMs         = 0);
    ///@}

    ///@warning Handling warning
    ///@name I2C Address
    ///@{
    /*!
      @brief Change device I2C address
      @param i2c_address I2C address
      @return True if successful

    */
    bool changeI2CAddress(const uint8_t i2c_address);
    /*!
      @brief Read device I2C address
      @param[out] i2c_address I2C address
      @return True if successful
    */
    bool readI2CAddress(uint8_t& i2c_address);
    ///@}

protected:
    bool start_periodic_measurement();
    bool start_periodic_measurement(const uint32_t interval,
                                    const kmeterISO::MeasurementUnit munit = kmeterISO::Celsius);
    bool stop_periodic_measurement();

    bool read_measurement(kmeterISO::Data& d, const kmeterISO::MeasurementUnit munit);
    bool read_internal_measurement(kmeterISO::Data& d, const kmeterISO::MeasurementUnit munit);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitKmeterISO, kmeterISO::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<kmeterISO::Data>> _data{};
    kmeterISO::MeasurementUnit _munit{kmeterISO::MeasurementUnit::Celsius};
    config_t _cfg{};
};

namespace kmeterISO {
namespace command {
///@cond
// See also https://github.com/m5stack/M5Unit-KMeterISO/blob/main/docs/Unit_KmeterISO_I2C_Protocol.pdf
constexpr uint8_t TEMPERATURE_CELSIUS_VAL_REG{0X00};
constexpr uint8_t TEMPERATURE_FAHRENHEIT_VAL_REG{0X04};
constexpr uint8_t INTERNAL_TEMPERATURE_CELSIUS_VAL_REG{0X10};
constexpr uint8_t INTERNAL_TEMPERATURE_FAHRENHEIT_VAL_REG{0X14};
constexpr uint8_t ERROR_STATUS_REG{0x20};
constexpr uint8_t TEMPERATURE_CELSIUS_STRING_REG{0x30};
constexpr uint8_t TEMPERATURE__FAHRENHEIT_STRING_REG{0x40};
constexpr uint8_t INTERNAL_TEMPERATURE_CELSIUS_STRING_REG{0x50};
constexpr uint8_t INTERNAL_TEMPERATURE_FAHRENHEIT_STRING_REG{0x60};
constexpr uint8_t FIRMWARE_VERSION_REG{0xFE};
constexpr uint8_t I2C_ADDRESS_REG{0xFF};
///@endcond
}  // namespace command
}  // namespace kmeterISO

}  // namespace unit
}  // namespace m5
#endif
