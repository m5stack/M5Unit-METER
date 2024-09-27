/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_KmeterISO.cpp
  @brief KmeterISO Unit for M5UnitUnified
*/
#include "unit_KmeterISO.hpp"
#include <M5Utility.hpp>
#include <array>
#include <thread>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::kmeterISO;
using namespace m5::unit::kmeterISO::command;

namespace {
template <typename T>
T array_to_type(const std::array<uint8_t, 4>& a)
{
    static_assert(std::is_integral<T>::value && sizeof(T) == 4, "Invalid type");
    return static_cast<T>((((uint32_t)a[3]) << 24) | (((uint32_t)a[2]) << 16) | (((uint32_t)a[1]) << 8) |
                          (((uint32_t)a[0]) << 0));
}

constexpr uint8_t reg_temperature_table[] = {
    TEMPERATURE_CELSIUS_VAL_REG,
    TEMPERATURE_FAHRENHEIT_VAL_REG,
};

constexpr uint8_t reg_internal_temperature_table[] = {
    INTERNAL_TEMPERATURE_CELSIUS_VAL_REG,
    INTERNAL_TEMPERATURE_FAHRENHEIT_VAL_REG,
};

}  // namespace

namespace m5 {
namespace unit {
// class UnitKmeterISO
const char UnitKmeterISO::name[] = "UnitKmeterISO";
const types::uid_t UnitKmeterISO::uid{"UnitKmeterISO"_mmh3};
const types::uid_t UnitKmeterISO::attr{0};

bool UnitKmeterISO::begin()
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

    uint8_t ver{0x00};
    if (!readFirmwareVersion(ver) && (ver == 0x00)) {
        M5_LIB_LOGE("Failed to read version");
        return false;
    }
    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.interval, _cfg.measurement_unit) : true;
}

void UnitKmeterISO::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = isReady() && read_measurement(d, _munit);
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitKmeterISO::start_periodic_measurement()
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    _periodic = true;
    _latest   = 0;
    return true;
}

bool UnitKmeterISO::start_periodic_measurement(const uint32_t interval, const MeasurementUnit munit)
{
    if (start_periodic_measurement()) {
        _interval = interval;
        _munit    = munit;
        return true;
    }
    return false;
}

bool UnitKmeterISO::stop_periodic_measurement()
{
    _periodic = _updated = false;
    return true;
}

bool UnitKmeterISO::readStatus(uint8_t& status)
{
    status = 0xFF;
    return readRegister8(ERROR_STATUS_REG, status, 0);
}

bool UnitKmeterISO::readFirmwareVersion(uint8_t& ver)
{
    return readRegister8(FIRMWARE_VERSION_REG, ver, 0);
}

bool UnitKmeterISO::measureSingleshot(kmeterISO::Data& d, kmeterISO::MeasurementUnit munit, const uint32_t timeoutMs)
{
    auto timeout_at = m5::utility::millis() + timeoutMs;
    do {
        if (isReady()) {
            return read_measurement(d, munit);
        }
        m5::utility::delay(1);
    } while (m5::utility::millis() <= timeout_at);

    M5_LIB_LOGW("Failed due to timeout");
    return false;
}

bool UnitKmeterISO::measureInternalSingleshot(kmeterISO::Data& d, kmeterISO::MeasurementUnit munit,
                                              const uint32_t timeoutMs)
{
    auto timeout_at = m5::utility::millis() + timeoutMs;
    do {
        if (isReady()) {
            return read_internal_measurement(d, munit);
        }
        m5::utility::delay(1);
    } while (m5::utility::millis() <= timeout_at);

    M5_LIB_LOGW("Failed due to timeout");
    return false;
}

bool UnitKmeterISO::changeI2CAddress(const uint8_t i2c_address)
{
    if (!m5::utility::isValidI2CAddress(i2c_address)) {
        M5_LIB_LOGE("Invalid address : %02X", i2c_address);
        return false;
    }
    if (writeRegister8(I2C_ADDRESS_REG, i2c_address) && changeAddress(i2c_address)) {
        // Wait wakeup
        uint8_t v{};
        bool done{};
        auto timeout_at = m5::utility::millis() + 1000;
        do {
            done = readRegister8(I2C_ADDRESS_REG, v, 0) && v == i2c_address;
            m5::utility::delay(1);
        } while (!done && m5::utility::millis() <= timeout_at);
        return done;
    }
    return false;
}

bool UnitKmeterISO::readI2CAddress(uint8_t& i2c_address)
{
    return readRegister8(I2C_ADDRESS_REG, i2c_address, 1);
}

//
bool UnitKmeterISO::read_measurement(Data& d, const kmeterISO::MeasurementUnit munit)
{
    assert(m5::stl::to_underlying(munit) < m5::stl::size(reg_temperature_table));
    return readRegister(reg_temperature_table[m5::stl::to_underlying(munit)], d.raw.data(), d.raw.size(), 0);
}

bool UnitKmeterISO::read_internal_measurement(Data& d, const kmeterISO::MeasurementUnit munit)
{
    assert(m5::stl::to_underlying(munit) < m5::stl::size(reg_internal_temperature_table));
    return readRegister(reg_internal_temperature_table[m5::stl::to_underlying(munit)], d.raw.data(), d.raw.size(), 0);
}

}  // namespace unit
}  // namespace m5
