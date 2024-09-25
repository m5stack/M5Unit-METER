/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_EEPROM.cpp
  @brief Accessor unit to EEPROM that holds calibration information
*/
#include "unit_EEPROM.hpp"
#include <M5Utility.h>

using namespace m5::utility::mmh3;
using namespace m5::unit::ads111x;

namespace {
constexpr uint8_t DATA_ADDRESS{0xD0};
constexpr Gain gain_table[] = {
    Gain::PGA_6144, Gain::PGA_4096, Gain::PGA_2048, Gain::PGA_1024, Gain::PGA_512, Gain::PGA_256,
};
};  // namespace

namespace m5 {
namespace unit {
namespace meter {

const char UnitEEPROM::name[] = "UnitEEPROMforMeter";
const types::uid_t UnitEEPROM::uid{"UnitEEPROMforMeter"_mmh3};
const types::uid_t UnitEEPROM::attr{0};

bool UnitEEPROM::readCalibration()
{
    int idx{};
    _calibration.fill({});

    for (auto&& e : gain_table) {
        assert(idx < _calibration.size() && "illegal index");
        if (!read_calibration(e, _calibration[idx].hope, _calibration[idx].actual)) {
            M5_LIB_LOGE("Failed to read calibration");
            return false;
        }
        M5_LIB_LOGV("Calibration[%u]: %d,%d", e, _calibration[idx].hope, _calibration[idx].actual);
        ++idx;
    }
    _calibration[6] = _calibration[7] = _calibration[5];  // 6,7 and BBB are the same as 5. see also Gain
    return true;
}

bool UnitEEPROM::read_calibration(const Gain gain, int16_t& hope, int16_t& actual)
{
    uint8_t reg = DATA_ADDRESS + m5::stl::to_underlying(gain) * 8;
    std::array<uint8_t, 8> buf{};
    hope = actual = 1;

    if (writeWithTransaction(reg, nullptr, 0U) != m5::hal::error::error_t::OK) {
        M5_LIB_LOGE("Failed to write");
        return false;
    }
    if (readWithTransaction(buf.data(), buf.size()) != m5::hal::error::error_t::OK) {
        return false;
    }

    uint8_t xorchk{};
    for (int_fast8_t i = 0; i < 5; ++i) {
        xorchk ^= buf[i];
    }
    if (xorchk != buf[5]) {
        return false;
    }

    m5::types::big_uint16_t hh(buf[1], buf[2]);
    m5::types::big_uint16_t aa(buf[3], buf[4]);
    hope   = (int16_t)hh.get();
    actual = (int16_t)aa.get();

    return true;
}

}  // namespace meter
}  // namespace unit
}  // namespace m5
#if 0
//A
======== [0]:1   -1,-1
======== [1]:1   -1,-1
======== [2]:1   -1,-1
======== [3]:1   -1,-1
======== [4]:1   6400,-6423
======== [5]:1   -1,-1
//V
======== [0]:1   -1,-1
======== [1]:1   7641,7613
======== [2]:1   -1,-1
======== [3]:1   -1,-1
======== [4]:1   5094,5073
======== [5]:1   -1,-1
#endif
