/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_INA226.cpp
  @brief INA226 Unit for M5UnitUnified
*/
#include "unit_INA226.hpp"
#include <M5Utility.hpp>
#include <array>
#include <thread>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::ina226;
using namespace m5::unit::ina226::command;
using m5::unit::types::elapsed_time_t;

namespace {
constexpr uint16_t MANUFACTURER_ID{0X5449};
constexpr uint16_t DIE_ID{0x2260};
constexpr uint16_t DEFAULT_CONFIG_VALUE{0x4127};

constexpr Mode mode_table[] = {
    Mode::PowerDown, Mode::ShuntVoltageSingle, Mode::BusVoltageSingle, Mode::ShuntAndBusSingle,
    Mode::PowerDown, Mode::ShuntVoltage,       Mode::BusVoltage,       Mode::ShuntAndBus,
};

/*
  _measureBits to Mode table
  MSB 0000 LSB
      |||+-- Shunt
      ||+--- Bus
      |+---- Power
      +----- Current
*/
constexpr Mode single_operation_table[] = {
    Mode::PowerDown,           // No measure
    Mode::ShuntVoltageSingle,  // Shunt
    Mode::BusVoltageSingle,    // Bus
    Mode::ShuntAndBusSingle,   // Bus, Shunt
    Mode::ShuntAndBusSingle,   // Power
    Mode::ShuntAndBusSingle,   // Power, Shunt
    Mode::ShuntAndBusSingle,   // Power, Bus
    Mode::ShuntAndBusSingle,   // Power, Bus, Shunt
    Mode::ShuntVoltageSingle,  // Current
    Mode::ShuntVoltageSingle,  // Current, Shunt
    Mode::ShuntAndBusSingle,   // Current, Bus
    Mode::ShuntAndBusSingle,   // Current, Bus, Shunt
    Mode::ShuntAndBusSingle,   // Current, Power
    Mode::ShuntAndBusSingle,   // Current, Power, Shunt
    Mode::ShuntAndBusSingle,   // Current, Power, Bus
    Mode::ShuntAndBusSingle,   // Current, Power, Bus, Shunt
};

constexpr Mode periodic_operation_table[] = {
    Mode::PowerDown,     // No measure
    Mode::ShuntVoltage,  // Shunt
    Mode::BusVoltage,    // Bus
    Mode::ShuntAndBus,   // Bus, Shunt
    Mode::ShuntAndBus,   // Power
    Mode::ShuntAndBus,   // Power, Shunt
    Mode::ShuntAndBus,   // Power, Bus
    Mode::ShuntAndBus,   // Power, Bus, Shunt
    Mode::ShuntVoltage,  // Current
    Mode::ShuntVoltage,  // Current, Shunt
    Mode::ShuntAndBus,   // Current, Bus
    Mode::ShuntAndBus,   // Current, Bus, Shunt
    Mode::ShuntAndBus,   // Current, Power
    Mode::ShuntAndBus,   // Current, Power, Shunt
    Mode::ShuntAndBus,   // Current, Power, Bus
    Mode::ShuntAndBus,   // Current, Power, Bus, Shunt
};

struct ModeCfg {
    explicit constexpr ModeCfg(const uint16_t val = 0) : v{val}
    {
    }
    inline Mode mode() const
    {
        return static_cast<Mode>(v & 0x07);
    }
    inline Sampling sampling() const
    {
        return static_cast<Sampling>((v >> 9) & 0x07);
    }
    inline ConversionTime busConversionTime() const
    {
        return static_cast<ConversionTime>((v >> 6) & 0x07);
    }
    inline ConversionTime shuntConversionTime() const
    {
        return static_cast<ConversionTime>((v >> 3) & 0x07);
    }

    inline void reset(bool b)
    {
        v = (v & ~(1U << 15)) | (b ? (1U << 15) : 0U);
    }
    inline void mode(const Mode m)
    {
        v = (v & ~0x07) | m5::stl::to_underlying(m);
    }
    inline void sampling(const Sampling rate)
    {
        v = (v & ~(0x07 << 9)) | (m5::stl::to_underlying(rate) << 9);
    }
    inline void busConversionTime(const ConversionTime ct)
    {
        v = (v & ~(0x07 << 6)) | (m5::stl::to_underlying(ct) << 6);
    }
    inline void shuntConversionTime(const ConversionTime ct)
    {
        v = (v & ~(0x07 << 3)) | (m5::stl::to_underlying(ct) << 3);
    }
    uint16_t v{};
};

struct Mask {
    static constexpr uint16_t ALERT_BITS_MASK{0xFC00};

    explicit Mask(const uint16_t mask = 0) : v{mask}
    {
    }
    inline bool SOL() const
    {
        return v & (1U << 15);
    }
    inline bool SUL() const
    {
        return v & (1U << 14);
    }
    inline bool BOL() const
    {
        return v & (1U << 13);
    }
    inline bool BUL() const
    {
        return v & (1U << 12);
    }
    inline bool POL() const
    {
        return v & (1U << 11);
    }
    inline bool CNVR() const
    {
        return v & (1U << 10);
    }
    inline bool AFF() const
    {
        return v & (1U << 4);
    }
    inline bool CVRF() const
    {
        return v & (1U << 3);
    }
    inline bool OVF() const
    {
        return v & (1U << 2);
    }
    inline bool APOL() const
    {
        return v & (1U << 1);
    }
    inline bool LEN() const
    {
        return v & (1U << 0);
    }
    inline uint16_t alertBits() const
    {
        return v & ALERT_BITS_MASK;
    }

    inline void SOL(const bool b)
    {
        v = (v & ~(1U << 15)) | (b ? (1U << 15) : 0);
    }
    inline void SUL(const bool b)
    {
        v = (v & ~(1U << 14)) | (b ? (1U << 14) : 0);
    }
    inline void BOL(const bool b)
    {
        v = (v & ~(1U << 13)) | (b ? (1U << 13) : 0);
    }
    inline void BUL(const bool b)
    {
        v = (v & ~(1U << 12)) | (b ? (1U << 12) : 0);
    }
    inline void POL(const bool b)
    {
        v = (v & ~(1U << 11)) | (b ? (1U << 11) : 0);
    }
    inline void CNVR(const bool b)
    {
        v = (v & ~(1U << 10)) | (b ? (1U << 10) : 0);
    }
    inline void AFF(const bool b)
    {
        v = (v & ~(1U << 4)) | (b ? (1U << 4) : 0);
    }
    inline void CVRF(const bool b)
    {
        v = (v & ~(1U << 3)) | (b ? (1U << 3) : 0);
    }
    inline void OVF(const bool b)
    {
        v = (v & ~(1U << 2)) | (b ? (1U << 2) : 0);
    }
    inline void APOL(const bool b)
    {
        v = (v & ~(1U << 1)) | (b ? (1U << 1) : 0);
    }
    inline void LEN(const bool b)
    {
        v = (v & ~(1U << 0)) | (b ? (1U << 0) : 0);
    }

    uint16_t v{};
};

float caluculate_currentLSB(const float maxCur)
{
    return maxCur / 32767.f;
}

uint16_t caluculate_calibration(const float shuntRes, const float maxCur, const float curLSB)
{
    return (0.00512f / (curLSB * shuntRes));
}

uint32_t calculate_interval(const uint16_t v /* config bits*/)
{
    static constexpr uint16_t rate_table[8] = {1, 4, 16, 64, 128, 256, 512, 1024};
    static constexpr uint16_t conv_table[8] = {140, 204, 332, 588, 1100, 2116, 4156, 8244};  // us
    static constexpr uint32_t overhead_per_sample_us{400};

    ModeCfg mc{v};
    uint32_t smp = rate_table[m5::stl::to_underlying(mc.sampling())];
    uint32_t bus = conv_table[m5::stl::to_underlying(mc.busConversionTime())];
    uint32_t sus = conv_table[m5::stl::to_underlying(mc.shuntConversionTime())];
    uint32_t us{};

    switch (mc.mode()) {
        case Mode::ShuntVoltageSingle:
        case Mode::ShuntVoltage:
            us = smp * sus;
            break;
        case Mode::BusVoltageSingle:
        case Mode::BusVoltage:
            us = smp * bus;
            break;
        case Mode::ShuntAndBusSingle:
        case Mode::ShuntAndBus:
            us = smp * (bus + sus);
            break;
        default:
            us = 0;
            break;
    }
    return (us + (smp * overhead_per_sample_us) + 999) / 1000;
}

}  // namespace

namespace m5 {
namespace unit {

// class UnitINA226
const char UnitINA226::name[] = "UnitINA226";
const types::uid_t UnitINA226::uid{"UnitINA226"_mmh3};
const types::attr_t UnitINA226::attr{attribute::AccessI2C};

UnitINA226::UnitINA226(const float shuntRes, const float maxCurA, const float curLSB, const uint8_t addr)
    : Component(addr),
      _data{new m5::container::CircularBuffer<ina226::Data>(1)},
      _shuntRes(shuntRes),
      _maxCurrentA{maxCurA},
      _currentLSB{curLSB}
{
    auto ccfg  = component_config();
    ccfg.clock = 400 * 1000U;
    component_config(ccfg);
    if (_currentLSB == 0.0f) {
        _currentLSB = caluculate_currentLSB(maxCurA);
    }
}

bool UnitINA226::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    // Check the validity of the currentLSB
    float calf = 0.00512f / (_currentLSB * _shuntRes);
    if (calf > 65535.0f) {
        M5_LIB_LOGE("currentLSB too small! %f CALF:%f", _currentLSB, calf);
        return false;
    }

    // Check unit
    uint16_t mid{}, did{};
    if (!readRegister16BE(MANUFACTURER_ID_REG, mid, 0)) {
        M5_LIB_LOGE("Failed to read Manufacturer ID %x", mid);
        return false;
    }
    if (!readRegister16BE(DIE_ID_REG, did, 0)) {
        M5_LIB_LOGE("Failed to read Die ID %x", did);
        return false;
    }
    if (mid != MANUFACTURER_ID || did != DIE_ID) {
        M5_LIB_LOGE("Illegal ID M:%x D:%x", mid, did);
        return false;
    }

    // reset
    if (!softReset()) {
        M5_LIB_LOGE("Failed to reset");
        return false;
    }

    // Set calibration
    uint16_t cal = caluculate_calibration(_shuntRes, _maxCurrentA, _currentLSB);
    if (!writeCalibration(cal)) {
        M5_LIB_LOGE("Failed to writeCalibration %u", cal);
        return false;
    }
    M5_LIB_LOGI("currentLSB:%f CAL:%u", _currentLSB, cal);

    //
    return powerDown() && (_cfg.start_periodic ? startPeriodicMeasurement(
                                                     _cfg.sampling_rate, _cfg.shunt_conversion_time,
                                                     _cfg.bus_conversion_time, _cfg.current, _cfg.voltage, _cfg.power)
                                               : true);
}

void UnitINA226::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = is_data_ready() && read_measurement(d);
            if (_updated) {
                _latest = m5::utility::millis();
                _data->push_back(d);
            }
        }
    }
}

bool UnitINA226::start_periodic_measurement(const bool current, const bool voltage, const bool power)
{
    if (inPeriodic()) {
        M5_LIB_LOGE("Periodic measurements are running");
        return false;
    }

    _measureBits = (current ? 8 : 0) | (voltage ? 2 : 0) | (power ? 4 : 0) | ((current || power) ? 1 : 0);
    if (!_measureBits) {
        return false;
    }

    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mc.mode(periodic_operation_table[_measureBits]);
        if (write_configuration(mc.v)) {
            _periodic = true;
            _latest   = 0;
            _interval = calculate_interval(mc.v);
        }
    }
    return _periodic;
}

bool UnitINA226::start_periodic_measurement(const ina226::Sampling rate, const ina226::ConversionTime sct,
                                            const ina226::ConversionTime bct, const bool current, const bool voltage,
                                            const bool power)
{
    ModeCfg mc{};
    if (!read_configuration(mc.v)) {
        return false;
    }
    mc.sampling(rate);
    mc.shuntConversionTime(sct);
    mc.busConversionTime(bct);
    return write_configuration(mc.v) && start_periodic_measurement(current, voltage, power);
}

bool UnitINA226::stop_periodic_measurement()
{
    if (inPeriodic()) {
        return powerDown();
    }
    return false;
}

bool UnitINA226::measureSingleshot(ina226::Data& data, const bool current, const bool voltage, const bool power)
{
    if (inPeriodic()) {
        M5_LIB_LOGW("Periodic measurements are running");
        return false;
    }

    _measureBits = (current ? 8 : 0) | (voltage ? 2 : 0) | (power ? 4 : 0) | ((current || power) ? 1 : 0);
    if (!_measureBits) {
        M5_LIB_LOGE("The measurement target is not specified");
        return false;
    }

    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mc.mode(single_operation_table[_measureBits]);
        auto waitMs = calculate_interval(mc.v);
        if (write_configuration(mc.v)) {
            m5::utility::delay(waitMs);
            auto timeout_at = m5::utility::millis() + 1000;
            do {
                if (is_data_ready() && read_measurement(data)) {
                    return true;
                }
                m5::utility::delay(1);
            } while (m5::utility::millis() <= timeout_at);
        }
    }
    return false;
}

bool UnitINA226::measureSingleshot(ina226::Data& data, const ina226::Sampling rate, const ina226::ConversionTime sct,
                                   const ina226::ConversionTime bct, const bool current, const bool voltage,
                                   const bool power)
{
    ModeCfg mc{};
    if (!read_configuration(mc.v)) {
        return false;
    }
    mc.sampling(rate);
    mc.shuntConversionTime(sct);
    mc.busConversionTime(bct);
    return write_configuration(mc.v) && measureSingleshot(data, current, voltage, power);
}

bool UnitINA226::readMode(ina226::Mode& mode)
{
    mode = Mode::PowerDown;
    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mode = mode_table[m5::stl::to_underlying(mc.mode())];
        return true;
    }
    return false;
}

bool UnitINA226::write_mode(const ina226::Mode mode)
{
    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mc.mode(mode);
        return write_configuration(mc.v);
    }
    return false;
}

bool UnitINA226::readSamplingRate(ina226::Sampling& rate)
{
    rate = Sampling::Rate1;
    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        rate = mc.sampling();
        return true;
    }
    return false;
}

bool UnitINA226::writeSamplingRate(const ina226::Sampling rate)
{
    if (inPeriodic()) {
        M5_LIB_LOGW("Periodic measurements are running");
        return false;
    }

    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mc.sampling(rate);
        return write_configuration(mc.v);
    }
    return false;
}

bool UnitINA226::readBusVoltageConversionTime(ina226::ConversionTime& ct)
{
    ct = ConversionTime::US_140;
    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        ct = mc.busConversionTime();
        return true;
    }
    return false;
}

bool UnitINA226::writeBusVoltageConversionTime(const ina226::ConversionTime ct)
{
    if (inPeriodic()) {
        M5_LIB_LOGW("Periodic measurements are running");
        return false;
    }

    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mc.busConversionTime(ct);
        return write_configuration(mc.v);
    }
    return false;
}

bool UnitINA226::readShuntVoltageConversionTime(ina226::ConversionTime& ct)
{
    ct = ConversionTime::US_140;
    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        ct = mc.shuntConversionTime();
        return true;
    }
    return false;
}

bool UnitINA226::writeShuntVoltageConversionTime(const ina226::ConversionTime ct)
{
    if (inPeriodic()) {
        M5_LIB_LOGW("Periodic measurements are running");
        return false;
    }

    ModeCfg mc{};
    if (read_configuration(mc.v)) {
        mc.shuntConversionTime(ct);
        return write_configuration(mc.v);
    }
    return false;
}

bool UnitINA226::read_mask(uint16_t& m)
{
    return readRegister16BE(MASK_REG, m, 0);
}

bool UnitINA226::write_mask(const uint16_t m)
{
    return writeRegister16BE(MASK_REG, m);
}

bool UnitINA226::read_configuration(uint16_t& v)
{
    v = 0;
    return readRegister16BE(CONFIGURATION_REG, v, 0);
}

bool UnitINA226::write_configuration(const uint16_t v)
{
    return writeRegister16BE(CONFIGURATION_REG, v);
}

bool UnitINA226::readCalibration(uint16_t& cal)
{
    cal = 0;
    return readRegister16BE(CALIBRATION_REG, cal, 0);
}

bool UnitINA226::writeCalibration(const uint16_t cal)
{
    return writeRegister16BE(CALIBRATION_REG, cal);
}

bool UnitINA226::powerDown()
{
    if (write_mode(Mode::PowerDown)) {
        _periodic = false;
        return true;
    }
    return false;
}

bool UnitINA226::softReset(const bool all)
{
    ModeCfg mc{};
    uint16_t cal{};
    _periodic = false;

    if (read_configuration(mc.v)) {
        mc.reset(true);
        if (write_configuration(mc.v)) {
            m5::utility::delay(2);
            if (read_configuration(mc.v) && mc.v == DEFAULT_CONFIG_VALUE && readCalibration(cal) && cal == 0) {
                _periodic = true;  // Default config register value is 0x4127 (measn Mode ShuntAndBus)
                return true;
            }
        }
    }
    return false;
}

bool UnitINA226::readAlertLimit(uint16_t& limit)
{
    return readRegister16BE(ALERT_LIMIT_REG, limit, 0);
}

bool UnitINA226::writeAlertLimit(const uint16_t limit)
{
    if (inPeriodic()) {
        M5_LIB_LOGW("Periodic measurements are running");
        return false;
    }
    return writeRegister16BE(ALERT_LIMIT_REG, limit);
}

bool UnitINA226::readAlertOccurred(bool& alert)
{
    alert = false;

    Mask mask{};
    if (read_mask(mask.v)) {
        alert = mask.AFF();
        return true;
    }
    return false;
}

bool UnitINA226::readAlert(ina226::Alert& type)
{
    static constexpr Alert type_table[] = {
        Alert::ConversionReady, Alert::PowerOver, Alert::BusUnder, Alert::BusOver, Alert::ShuntUnder, Alert::ShuntOver,
    };

    type = Alert::Unknown;

    Mask mask{};
    if (read_mask(mask.v)) {
        auto alert_bits = mask.alertBits();

        if (alert_bits && (alert_bits & (alert_bits - 1))) {
            M5_LIB_LOGW("Multiple Alert bits are standing");
            return true;
        }
        if (!alert_bits) {
            type = Alert::None;
            return true;
        }

        alert_bits >>= 10;
        for (uint_fast8_t i = 0; i < m5::stl::size(type_table); ++i) {
            if ((1U << i) & alert_bits) {
                type = type_table[i];
                return true;
            }
        }
    }
    return false;
}

bool UnitINA226::writeAlert(const ina226::Alert type, const uint16_t limit, const bool latch)
{
    if (inPeriodic()) {
        M5_LIB_LOGW("Periodic measurements are running");
        return false;
    }

    Mask mask{};
    if (read_mask(mask.v)) {
        mask.v &= ~Mask::ALERT_BITS_MASK;
        switch (type) {
            case Alert::ShuntOver:
                mask.SOL(true);
                break;
            case Alert::ShuntUnder:
                mask.SUL(true);
                break;
            case Alert::BusOver:
                mask.BOL(true);
                break;
            case Alert::BusUnder:
                mask.BUL(true);
                break;
            case Alert::PowerOver:
                mask.POL(true);
                break;
            case Alert::ConversionReady:
                mask.CNVR(true);
                break;
            default:
                break;
        }
        if (!mask.alertBits()) {
            M5_LIB_LOGE("Illegal type %u", type);
            return false;
        }
        mask.LEN(latch);
        return writeAlertLimit(limit) && write_mask(mask.v);
    }
    return false;
}

//
bool UnitINA226::is_data_ready()
{
    Mask mask{};
    return read_mask(mask.v) && mask.CVRF() && !mask.OVF();
}

bool UnitINA226::read_measurement(ina226::Data& d)
{
    bool ret{true};
    uint8_t reg{SHUNT_VOLTAGE_REG};  // 0x01
    for (uint_fast8_t i = 0; i < 4; ++i) {
        if (_measureBits & (1U << i)) {
            ret &= readRegister16BE((uint8_t)(reg + i), d.raw[i], 0);  // reg 0x01 - 0x04
        }
    }
    d.currentLSB = _currentLSB;
    return _measureBits && ret;
}

// class UnitINA226_10A
const char UnitINA226_10A::name[] = "UnitINA226_10A";
const types::uid_t UnitINA226_10A::uid{"UnitINA226_10A"_mmh3};
const types::attr_t UnitINA226_10A::attr{attribute::AccessI2C};

// class UnitINA226_1A
const char UnitINA226_1A::name[] = "UnitINA226_1A";
const types::uid_t UnitINA226_1A::uid{"UnitINA226_1A"_mmh3};
const types::attr_t UnitINA226_1A::attr{attribute::AccessI2C};

}  // namespace unit
}  // namespace m5
