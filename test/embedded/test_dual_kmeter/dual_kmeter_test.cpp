/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for DualKmeter
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_DualKmeter.hpp>
#include <chrono>
#include <thread>
#include <iostream>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::dual_kmeter;
using namespace m5::unit::dual_kmeter::command;
using m5::unit::types::elapsed_time_t;

namespace in_wire {
template <uint32_t FREQ, uint32_t WNUM = 0>
class GlobalFixture : public ::testing::Environment {
    static_assert(WNUM < 2, "Wire number must be lesser than 2");

public:
    void SetUp() override
    {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::in_i2c_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::in_i2c_scl);
        TwoWire* w[2]    = {&Wire, &Wire1};
        if (WNUM < m5::stl::size(w) && i2cIsInit(WNUM)) {
            M5_LOGW("Already inititlized Wire %d. Terminate and restart FREQ %u", WNUM, FREQ);
            w[WNUM]->end();
        }
        w[WNUM]->begin(pin_num_sda, pin_num_scl, FREQ);
    }
};
}  // namespace in_wire

const ::testing::Environment* global_fixture =
    ::testing::AddGlobalTestEnvironment(new in_wire::GlobalFixture<100000U>());

constexpr uint32_t STORED_SIZE{6};

class TestDualKmeter : public ComponentTestBase<UnitDualKmeter, bool> {
protected:
    virtual UnitDualKmeter* get_instance() override
    {
        auto ptr         = new m5::unit::UnitDualKmeter();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestDualKmeter, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestDualKmeter, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestDualKmeter, ::testing::Values(false));

namespace {
constexpr MeasurementUnit mu_table[] = {MeasurementUnit::Celsius, MeasurementUnit::Fahrenheit};
constexpr Channel ch_table[]         = {Channel::One, Channel::Two};
constexpr uint32_t interval_table[]  = {10, 100, 500};

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times, const uint32_t measure_duration = 0)
{
    auto tm         = unit->interval();
    auto timeout_at = m5::utility::millis() + 10 * 1000;

    do {
        unit->update();
        if (unit->updated()) {
            break;
        }
        std::this_thread::yield();
    } while (!unit->updated() && m5::utility::millis() <= timeout_at);
    // timeout
    if (!unit->updated()) {
        return 0;
    }

    //
    uint32_t measured{};
    auto start_at = m5::utility::millis();
    timeout_at    = start_at + (times * (tm + measure_duration) * 2);

    do {
        unit->update();
        measured += unit->updated() ? 1 : 0;
        if (measured >= times) {
            break;
        }
        // std::this_thread::yield();
        m5::utility::delay(1);

    } while (measured < times && m5::utility::millis() <= timeout_at);
    return (measured == times) ? m5::utility::millis() - start_at : 0;
    // return (measured == times) ? unit->updatedMillis() - start_at : 0;
}
}  // namespace

TEST_P(TestDualKmeter, Basic)
{
    SCOPED_TRACE(ustr);

    // Version
    uint8_t ver{0x00};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);

    // Measure unit
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Celsius);

    unit->setMeasurementUnit(MeasurementUnit::Fahrenheit);
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Fahrenheit);
    unit->setMeasurementUnit(MeasurementUnit::Celsius);
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Celsius);

    // Measure channel
    Channel ch{};
    EXPECT_TRUE(unit->readCurrentChannel(ch));
    EXPECT_EQ(ch, Channel::One);

    for (auto&& c : ch_table) {
        EXPECT_TRUE(unit->writeCurrentChannel(c));
        EXPECT_TRUE(unit->readCurrentChannel(ch));
        EXPECT_EQ(ch, c);
    }
}

TEST_P(TestDualKmeter, Singleshot)
{
    SCOPED_TRACE(ustr);

    Data d{};

    // Failed in periodic
    EXPECT_TRUE(unit->inPeriodic());
    for (auto&& c : ch_table) {
        for (auto&& mu : mu_table) {
            EXPECT_FALSE(unit->measureSingleshot(d, c, mu));
            EXPECT_FALSE(unit->measureInternalSingleshot(d, c, mu));
        }
    }

    //
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& c : ch_table) {
        EXPECT_TRUE(unit->writeCurrentChannel(c));
        for (auto&& sc : ch_table) {
            for (auto&& mu : mu_table) {
                uint32_t cnt{4};
                while (cnt--) {
                    EXPECT_TRUE(unit->measureSingleshot(d, sc, mu));
                    EXPECT_TRUE(std::isfinite(d.temperature()));
                    EXPECT_EQ(d.channel, sc);
                    // M5_LOGI("T%d:%f", (int)d.channel + 1, d.temperature());
                    m5::utility::delay(100);
                }
                cnt = 4;
                while (cnt--) {
                    EXPECT_TRUE(unit->measureInternalSingleshot(d, sc, mu));
                    EXPECT_TRUE(std::isfinite(d.temperature()));
                    EXPECT_EQ(d.channel, sc);
                    // M5_LOGI("IT%d:%f", (int)d.channel + 1, d.temperature());
                    m5::utility::delay(100);
                }
            }
            Channel ac{};
            EXPECT_TRUE(unit->readCurrentChannel(ac));
            EXPECT_EQ(ac, c);
        }
    }
}

TEST_P(TestDualKmeter, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& it : interval_table) {
        for (auto&& c : ch_table) {
            for (auto&& mu : mu_table) {
                EXPECT_TRUE(unit->startPeriodicMeasurement(it, c, mu));
                EXPECT_TRUE(unit->inPeriodic());

                auto elapsed = test_periodic(unit.get(), STORED_SIZE);
                EXPECT_TRUE(unit->stopPeriodicMeasurement());
                EXPECT_FALSE(unit->inPeriodic());

                EXPECT_GE(elapsed, it * STORED_SIZE);

                EXPECT_EQ(unit->available(), STORED_SIZE);
                EXPECT_FALSE(unit->empty());
                EXPECT_TRUE(unit->full());

                uint32_t cnt{STORED_SIZE / 2};
                while (cnt-- && unit->available()) {
                    EXPECT_TRUE(std::isfinite(unit->temperature()));
                    EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
                    EXPECT_FALSE(unit->empty());
                    // M5_LOGI("T%d:%f", (int)unit->oldest().channel + 1, unit->temperature());
                    unit->discard();
                }
                EXPECT_EQ(unit->available(), STORED_SIZE / 2);
                EXPECT_FALSE(unit->empty());
                EXPECT_FALSE(unit->full());

                unit->flush();
                EXPECT_EQ(unit->available(), 0);
                EXPECT_TRUE(unit->empty());
                EXPECT_FALSE(unit->full());

                EXPECT_FALSE(std::isfinite(unit->temperature()));
            }
        }
    }
}
