/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for KMeterISO
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_KmeterISO.hpp>
#include <chrono>
#include <thread>
#include <iostream>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::kmeter_iso;
using namespace m5::unit::kmeter_iso::command;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<100000U>());

constexpr uint32_t STORED_SIZE{6};

class TestKmeterISO : public ComponentTestBase<UnitKmeterISO, bool> {
protected:
    virtual UnitKmeterISO* get_instance() override
    {
        auto ptr         = new m5::unit::UnitKmeterISO();
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestKmeterISO, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestKmeterISO, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestKmeterISO, ::testing::Values(false));

namespace {

constexpr MeasurementUnit mu_table[] = {MeasurementUnit::Celsius, MeasurementUnit::Fahrenheit};
constexpr uint32_t interval_table[]  = {20, 100, 500};

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

TEST_P(TestKmeterISO, Basic)
{
    SCOPED_TRACE(ustr);

    // Version
    uint8_t ver{0x00};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);

    // Properties
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Celsius);

    unit->setMeasurementUnit(MeasurementUnit::Fahrenheit);
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Fahrenheit);
    unit->setMeasurementUnit(MeasurementUnit::Celsius);
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Celsius);
}

TEST_P(TestKmeterISO, Singleshot)
{
    SCOPED_TRACE(ustr);
    Data d{};

    // Failed in peiordic
    EXPECT_TRUE(unit->inPeriodic());
    for (auto&& mu : mu_table) {
        EXPECT_FALSE(unit->measureSingleshot(d, mu));
    }

    //
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& mu : mu_table) {
        uint32_t cnt{8};
        while (cnt--) {
            EXPECT_TRUE(unit->measureSingleshot(d, mu, 1000));
            EXPECT_TRUE(std::isfinite(d.temperature()));
            // M5_LOGI("T:%f", d.temperature());
            m5::utility::delay(100);
        }
    }
}

TEST_P(TestKmeterISO, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& it : interval_table) {
        for (auto&& mu : mu_table) {
            std::string s{};
            s = m5::utility::formatString("Munit:%u %u", m5::stl::to_underlying(mu), it);
            SCOPED_TRACE(s.c_str());

            EXPECT_TRUE(unit->startPeriodicMeasurement(it, mu));
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
                M5_LOGI("T:%f", unit->temperature());
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

/*
  WARNING!!
  Failure of this test will result in an unexpected I2C address being set!
*/
TEST_P(TestKmeterISO, I2CAddress)
{
    SCOPED_TRACE(ustr);

    // Periodic failed
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->changeI2CAddress(0x07));
    EXPECT_FALSE(unit->changeI2CAddress(0x10));

    //
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    Data d{};
    uint8_t ver{}, addr{};

    EXPECT_FALSE(unit->changeI2CAddress(0x07));  // Invalid
    EXPECT_FALSE(unit->changeI2CAddress(0x78));  // Invalid

    // Change to 0x10
    EXPECT_TRUE(unit->changeI2CAddress(0x10));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x10);
    EXPECT_EQ(unit->address(), 0x10);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));

    // Change to 0x77
    EXPECT_TRUE(unit->changeI2CAddress(0x77));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x77);
    EXPECT_EQ(unit->address(), 0x77);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));

    // Change to 0x52
    EXPECT_TRUE(unit->changeI2CAddress(0x52));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x52);
    EXPECT_EQ(unit->address(), 0x52);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));

    // Change to default
    EXPECT_TRUE(unit->changeI2CAddress(UnitKmeterISO::DEFAULT_ADDRESS));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, +UnitKmeterISO::DEFAULT_ADDRESS);
    EXPECT_EQ(unit->address(), +UnitKmeterISO::DEFAULT_ADDRESS);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));
}
