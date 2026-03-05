/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for DualKmeter
  NOTE: A thermocouple must be connected to the unit for tests to pass.
        Without it, the STM32 never reports data-ready and all measurements time out.
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_DualKmeter.hpp>
#include <chrono>
#include <iostream>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::dual_kmeter;
using namespace m5::unit::dual_kmeter::command;
using m5::unit::types::elapsed_time_t;

constexpr uint32_t STORED_SIZE{6};

class TestDualKmeter : public I2CComponentTestBase<UnitDualKmeter> {
protected:
    virtual UnitDualKmeter* get_instance() override
    {
        auto ptr         = new m5::unit::UnitDualKmeter();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
    // DualKmeter uses M5-Bus internal I2C
    virtual bool begin() override
    {
        M5_LOGI("Using M5.In_I2C");
        return Units.add(*unit, M5.In_I2C) && Units.begin();
    }
};

namespace {
constexpr MeasurementUnit mu_table[] = {MeasurementUnit::Celsius, MeasurementUnit::Fahrenheit};
constexpr Channel ch_table[]         = {Channel::One, Channel::Two};
constexpr uint32_t interval_table[]  = {10, 100, 500};
}  // namespace

TEST_F(TestDualKmeter, Basic)
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

TEST_F(TestDualKmeter, Singleshot)
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

TEST_F(TestDualKmeter, Periodic)
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

                auto r = collect_periodic_measurements(unit.get(), STORED_SIZE);
                EXPECT_FALSE(r.timed_out);
                EXPECT_EQ(r.update_count, STORED_SIZE);
                EXPECT_LE(r.median(), r.expected_interval + 1);

                EXPECT_TRUE(unit->stopPeriodicMeasurement());
                EXPECT_FALSE(unit->inPeriodic());

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
