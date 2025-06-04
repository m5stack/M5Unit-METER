/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_DualKmeter.cpp
  @brief DualKmeter Unit for M5UnitUnified
*/
#include "unit_DualKmeter.hpp"
#include <M5Utility.hpp>
#include <array>
#include <thread>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::dual_kmeter;
using namespace m5::unit::dual_kmeter::command;
using m5::unit::types::elapsed_time_t;

namespace {
constexpr uint8_t reg_temperature_table[] = {
    TEMPERATURE_CELSIUS_REG,
    TEMPERATURE_FAHRENHEIT_REG,
};

constexpr uint8_t reg_internal_temperature_table[] = {
    INTERNAL_TEMPERATURE_CELSIUS_REG,
    INTERNAL_TEMPERATURE_FAHRENHEIT_REG,
};

}  // namespace

namespace m5 {
namespace unit {
// class UnitDualKmeter
const char UnitDualKmeter::name[] = "UnitDualKmeter";
const types::uid_t UnitDualKmeter::uid{"UnitDualKmeter"_mmh3};
const types::attr_t UnitDualKmeter::attr{attribute::AccessI2C};

bool UnitDualKmeter::begin()
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

    uint8_t ver{};
    if (!readFirmwareVersion(ver) && (ver == 0x00)) {
        M5_LIB_LOGE("Failed to read version %02X", ver);
        return false;
    }
    M5_LIB_LOGD("FW:%02X", ver);

    return _cfg.start_periodic
               ? startPeriodicMeasurement(_cfg.interval, _cfg.measurement_channel, _cfg.measurement_unit)
               : true;
}

void UnitDualKmeter::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = is_data_ready() && read_measurement(d, _munit);
            if (_updated) {
                _latest   = m5::utility::millis();
                d.channel = _channel;
                _data->push_back(d);
            }
        }
    }
}

bool UnitDualKmeter::start_periodic_measurement()
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    _periodic = true;
    _latest   = 0;
    return _periodic;
}

bool UnitDualKmeter::start_periodic_measurement(const uint32_t interval, const Channel channel,
                                                const MeasurementUnit munit)
{
    if (writeCurrentChannel(channel) && start_periodic_measurement()) {
        _interval = interval;
        _munit    = munit;
        return true;
    }
    return false;
}

bool UnitDualKmeter::stop_periodic_measurement()
{
    _periodic = _updated = false;
    return true;
}

bool UnitDualKmeter::readStatus(uint8_t& status)
{
    status = 0xFF;
    return read_register8(STATUS_REG, status);
}

bool UnitDualKmeter::readFirmwareVersion(uint8_t& ver)
{
    ver = 0;
    return read_register8(FIRMWARE_VERSION_REG, ver);
}

bool UnitDualKmeter::measureSingleshot(dual_kmeter::Data& d, const dual_kmeter::Channel channel,
                                       const dual_kmeter::MeasurementUnit munit, const uint32_t timeoutMs)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    d.channel = channel;

    auto prev_ch = _channel;
    if (writeCurrentChannel(channel)) {
        auto timeout_at = m5::utility::millis() + timeoutMs;
        do {
            if (is_data_ready()) {
                return read_measurement(d, munit) && writeCurrentChannel(prev_ch);
            }
            m5::utility::delay(1);
        } while (m5::utility::millis() <= timeout_at);
        M5_LIB_LOGW("Failed due to timeout");
    }
    return false;
}

bool UnitDualKmeter::measureInternalSingleshot(dual_kmeter::Data& d, const dual_kmeter::Channel channel,
                                               const dual_kmeter::MeasurementUnit munit, const uint32_t timeoutMs)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    d.channel = channel;

    auto prev_ch = _channel;
    if (writeCurrentChannel(channel)) {
        auto timeout_at = m5::utility::millis() + timeoutMs;
        do {
            if (is_data_ready()) {
                return read_internal_measurement(d, munit) && writeCurrentChannel(prev_ch);
            }
            m5::utility::delay(1);
        } while (m5::utility::millis() <= timeout_at);
        M5_LIB_LOGW("Failed due to timeout");
    }
    return false;
}

bool UnitDualKmeter::readCurrentChannel(dual_kmeter::Channel& channel)
{
    uint8_t v{};
    if (read_register8(CHANNEL_REG, v) && (v < 2)) {
        channel = static_cast<Channel>(v);
        return true;
    }
    return false;
}

bool UnitDualKmeter::writeCurrentChannel(const dual_kmeter::Channel channel)
{
    if (writeRegister8(CHANNEL_REG, m5::stl::to_underlying(channel))) {
        _channel = channel;
        return true;
    }
    return false;
}

//
bool UnitDualKmeter::read_measurement(Data& d, const dual_kmeter::MeasurementUnit munit)
{
    return read_register(reg_temperature_table[m5::stl::to_underlying(munit)], d.raw.data(), d.raw.size());
}

bool UnitDualKmeter::read_internal_measurement(Data& d, const dual_kmeter::MeasurementUnit munit)
{
    return read_register(reg_internal_temperature_table[m5::stl::to_underlying(munit)], d.raw.data(), d.raw.size());
}

}  // namespace unit
}  // namespace m5
