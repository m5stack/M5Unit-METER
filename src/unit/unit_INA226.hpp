/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_INA226.hpp
  @brief INA226 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_METER_UNIT_INA226_HPP
#define M5_UNIT_METER_UNIT_INA226_HPP

#include <M5UnitComponent.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

namespace ina226 {

/*!
  @enum Sampling
  @brief The number of samples that are collected and averaged
 */
enum class Sampling : uint8_t {
    Rate1,     //!< 1 sps (as default)
    Rate4,     //!< 4 sps
    Rate16,    //!< 16 sps
    Rate64,    //!< 64 sps
    Rate128,   //!< 128 sps
    Rate256,   //!< 256 sps
    Rate512,   //!< 512 sps
    Rate1024,  //!< 1024 sps
};

/*!
  @enum ConversionTime
  @brief The conversion time for the bus voltage measurement
 */
enum class ConversionTime : uint8_t {
    US_140,   //!< 140 us
    US_204,   //!< 204 us
    US_332,   //!< 332 us
    US_588,   //!< 588 us
    US_1100,  //!< 1.1 ms (as default)
    US_2116,  //!< 2.116 ms
    US_4156,  //!< 4.156 ms
    US_8244,  //!< 8.244 ms
};

/*!
  @enum Mode
  @brief Operation mode
 */
enum class Mode : uint8_t {
    PowerDown,           //!< Power-Down (or Shutdown)
    ShuntVoltageSingle,  //!< Shunt Voltage, Triggered
    BusVoltageSingle,    //!< Bus Voltage, Triggered
    ShuntAndBusSingle,   //!< Shunt and Bus, Triggered
    // 0x04 Same as PowerDown
    ShuntVoltage = 0x05,  //!< Shunt Voltage, Continuous
    BusVoltage,           //!< Bus Voltage, Continuous
    ShuntAndBus,          //!< Shunt and Bus, Continuous (as default)
};

/*!
  @enum Alert
  @brief Alert type
 */
enum class Alert : int8_t {
    Unknown = -1,
    None,             //!< No alert
    ShuntOver,        //!< Shunt Voltage Over-Voltage
    ShuntUnder,       //!< Shunt Voltage Under-Voltage
    BusOver,          //!< Bus Voltage Over-Voltage
    BusUnder,         //!< Bus Voltage Under-Voltage
    PowerOver,        //!< Power Over-limit
    ConversionReady,  //!< Conversion Ready
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint16_t, 4> raw{};  //!< Raw data 0:Shunt 1:Bus 2:Power 3:Current
    float currentLSB{};             //!< currentLSB

    //@brirf Sunt voltage (mV)
    inline float shuntVoltage() const
    {
        return raw[0] * 0.0025f;
    }
    // @brief Bus voltage (mV)
    inline float voltage() const
    {
        return raw[1] * 1.25f;
    }
    // @brief Power (mW)
    inline float power() const
    {
        return raw[2] * currentLSB * 25.f * 1000.f;
    }
    // @brief Current (mA)
    inline float current() const
    {
        return (int16_t)raw[3] * currentLSB * 1000.f;
    }
};

}  // namespace ina226

/*!
  @class UnitINA226
  @brief Base class for INA226 unit
 */
class UnitINA226 : public Component, public PeriodicMeasurementAdapter<UnitINA226, ina226::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitINA226, 0x00);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Measure current?
        bool current{true};
        //! Measure bus voltage?
        bool voltage{true};
        //! Measure power?
        bool power{true};
        //! Sampling rate
        ina226::Sampling sampling_rate{ina226::Sampling::Rate16};
        //! Shunt conversion time
        ina226::ConversionTime shunt_conversion_time{ina226::ConversionTime::US_1100};
        //! Bus conversion time
        ina226::ConversionTime bus_conversion_time{ina226::ConversionTime::US_1100};
    };

protected:
    /*!
      @brief Constructor
      @param shuntRes Shunt resistor (O)
      @param maxCurA Maximum measure current (A)
      @paream curLSB currentLSB
     */
    UnitINA226(const float shuntRes, const float maxCurA, const float curLSB, const uint8_t addr = DEFAULT_ADDRESS);

public:
    virtual ~UnitINA226()
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
    //@brief Gets the shunt Resistor (O)
    inline float shuntResistor() const
    {
        return _shuntRes;
    }
    //! @brief Gets the maximum current (A)
    inline float maximumCurrent() const
    {
        return _maxCurrentA;
    }
    //! @brief Gets the current LSB
    inline float currentLSB() const
    {
        return _currentLSB;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest shunt voltage (mV)
    inline float shuntVoltage() const
    {
        return !empty() ? oldest().shuntVoltage() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest Bus voltage (mV)
    inline float voltage() const
    {
        return !empty() ? oldest().voltage() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest power (mW)
    inline float power() const
    {
        return !empty() ? oldest().power() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest current (mA)
    inline float current() const
    {
        return !empty() ? oldest().current() : std::numeric_limits<float>::quiet_NaN();
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement in the current settings
      @param current Measure current if true
      @param voltage Measure bus voltage if true
      @param power Measure power if true
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const bool current = true, const bool voltage = true, const bool power = true)
    {
        return PeriodicMeasurementAdapter<UnitINA226, ina226::Data>::startPeriodicMeasurement(current, voltage, power);
    }
    /*!
      @brief Start periodic measurement
      @param rate Sampling Sampling rate
      @paran sct Shunt conversion time
      @paran bct Bus conversion time
      @param current Measure current if true
      @param voltage Measure bus voltage if true
      @param power Measure power if true
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const ina226::Sampling rate, const ina226::ConversionTime sct,
                                         const ina226::ConversionTime bct, const bool current = true,
                                         const bool voltage = true, const bool power = true)
    {
        return PeriodicMeasurementAdapter<UnitINA226, ina226::Data>::startPeriodicMeasurement(rate, sct, bct, current,
                                                                                              voltage, power);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitINA226, ina226::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @details Measuring in the current settings
      @param[out] data Measuerd data
      @param current Measure current if true
      @param voltage Measure bus voltage if true
      @param power Measure power if true
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Until it can be measured, it will be blocked until the timeout time
    */
    bool measureSingleshot(ina226::Data& data, const bool current = true, const bool voltage = true,
                           const bool power = true);
    /*!
      @brief Measurement single shot
      @details Measuring in the current settings
      @param[out] data Measuerd data
      @param rate Sampling Sampling rate
      @paran sct Shunt conversion time
      @paran bct Bus conversion time
      @param current Measure current if true
      @param voltage Measure bus voltage if true
      @param power Measure power if true
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Until it can be measured, it will be blocked until the timeout time
    */
    bool measureSingleshot(ina226::Data& data, const ina226::Sampling rate, const ina226::ConversionTime sct,
                           const ina226::ConversionTime bct, const bool current = true, const bool voltage = true,
                           const bool power = true);

    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Read the operation mode
      @param[out] mode Mode
      @return True if successful
     */
    bool readMode(ina226::Mode& mode);
    /*!
      @brief Read the sampling rate
      @param[out] rate Samling rate
      @return True if successful
     */
    bool readSamplingRate(ina226::Sampling& rate);
    /*!
      @brief Write the sampling rate
      @param rate Samling rate
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool writeSamplingRate(const ina226::Sampling rate);
    /*!
      @brief Read the convsrsion time of Bus voltage
      @param[out] ct Conversion time
      @return True if successful
     */
    bool readBusVoltageConversionTime(ina226::ConversionTime& ct);
    /*!
      @brief Write the convsrsion time of Bus voltage
      @paramct Conversion time
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool writeBusVoltageConversionTime(const ina226::ConversionTime ct);
    /*!
      @brief Read the convsrsion time of Shunt voltage
      @param[out] ct Conversion time
      @return True if successful
     */
    bool readShuntVoltageConversionTime(ina226::ConversionTime& ct);
    /*!
      @brief Write the convsrsion time of Shunt voltage
      @paramct Conversion time
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool writeShuntVoltageConversionTime(const ina226::ConversionTime ct);
    /*!
      @brief Read the calibration value
      @param[out] cal Calibration value
      @return True if successful
     */
    bool readCalibration(uint16_t& cal);
    /*!
      @brief Write the calibration value
      @param cal Calibration value
      @return True if successful
     */
    bool writeCalibration(const uint16_t cal);
    /*!
      @brief Read the alert type
      @param[out] type Alert type
      @return True if successful
     */
    bool readAlert(ina226::Alert& type);
    /*!
      @brief Write the alert type and limit value
      @param type Alert type
      @param limit Limit value (Ignore if type is Alert::ConversionReady)
      @param latch Alert latch enabled if true
      @return True if successful
      @note The unit of value depends on the type of Alert (See also datasheet)
      |Type|Alert|Description|Unit|Value|
      |---|---|---|---|---|
      |Type::ShuntOver Type::ShuntUnder| SOL/SUL|Shunt over/under limit|V| V / 0.00125 |
      |Type::BusOver Type::BusUnder | BOL/BUL|Bus over/under limit|V| V / 0..00125 |
      |Type::PowerOver | POL|Power over limit|W| W / (25 ×  currentLSB) |
      |Type::ConversionReady | CNVR | Conversion ready| - | - |
      @warning During periodic detection runs, an error is returned
     */
    bool writeAlert(const ina226::Alert type, const uint16_t limit, const bool latch = true);
    /*!
      @brief Read the alerm limit value
      @param[out] limit Limit value
      @return True if successful
      @note The unit of value depends on the type of Alert (See also datasheet)
     */
    bool readAlertLimit(uint16_t& limit);
    /*!
      @brief Write the alerm limit value
      @param limit Limit value
      @return True if successful
      @note The unit of value depends on the type of Alert (See also datasheet)
      @warning During periodic detection runs, an error is returned
    */
    bool writeAlertLimit(const uint16_t limit);
    ///@}

    /*!
      @brief Read the alert occurred?
      @param[out] alert An alert occurred if true
      @return True if successful
     */
    bool readAlertOccurred(bool& alert);
    /*!
      @brief Power down
      @return True if successful
     */
    bool powerDown();
    /*!
      @brief Software reset
      @param all Rewrite calibration value if false
      @return True if successful
      @warning All register values are changed to their initial values
     */
    bool softReset(const bool all = false);

protected:
    bool start_periodic_measurement(const bool current, const bool voltage, const bool power);
    bool start_periodic_measurement(const ina226::Sampling rate, const ina226::ConversionTime sct,
                                    const ina226::ConversionTime bct, const bool current, const bool voltage,
                                    const bool power);
    bool stop_periodic_measurement();

    bool write_mode(const ina226::Mode mode);
    bool read_configuration(uint16_t& v);
    bool write_configuration(const uint16_t v);
    bool read_mask(uint16_t& m);
    bool write_mask(const uint16_t m);

    bool is_data_ready();
    bool read_measurement(ina226::Data& d);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitINA226, ina226::Data);

private:
    std::unique_ptr<m5::container::CircularBuffer<ina226::Data>> _data{};
    config_t _cfg{};
    float _shuntRes{}, _maxCurrentA{}, _currentLSB{};
    uint8_t _measureBits{};  // LSB 0:Shunt 1:Bus 2:Power 3:Current MSB
};

/*!
  @class UnitINA226_10A
  @brief UnitINA226_10A unit
 */
class UnitINA226_10A : public UnitINA226 {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitINA226_10A, 0x41);

public:
    /*!
      @param curLSB currentLSB (calculated and set internally if zero)
     */
    explicit UnitINA226_10A(const float curLSB = 0.0f)
        : UnitINA226(0.005f /* 5mR */, 10.0f /* 10A */, curLSB, DEFAULT_ADDRESS)
    {
    }
    virtual ~UnitINA226_10A()
    {
    }
};

/*!
  @class UnitINA226_1A
  @brief UnitINA226_1A unit
 */
class UnitINA226_1A : public UnitINA226 {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitINA226_1A, 0x41);

public:
    /*!
      @param curLSB currentLSB (calculated and set internally if zero)
     */
    explicit UnitINA226_1A(const float curLSB = 0.0f)
        : UnitINA226(0.080f /* 80mR */, 1.0f /* 1A */, curLSB, DEFAULT_ADDRESS)
    {
    }
    virtual ~UnitINA226_1A()
    {
    }
};

namespace ina226 {
///@cond
namespace command {
constexpr uint8_t CONFIGURATION_REG{0x00};

constexpr uint8_t SHUNT_VOLTAGE_REG{0x01};
constexpr uint8_t BUS_VOLTAGE_REG{0x02};
constexpr uint8_t POWER_REG{0x03};
constexpr uint8_t CURRENT_REG{0x04};

constexpr uint8_t CALIBRATION_REG{0X05};
constexpr uint8_t MASK_REG{0x06};
constexpr uint8_t ALERT_LIMIT_REG{0x07};

constexpr uint8_t MANUFACTURER_ID_REG{0xFE};
constexpr uint8_t DIE_ID_REG{0XFF};
}  // namespace command
///@endcond
}  // namespace ina226

}  // namespace unit
}  // namespace m5
#endif
