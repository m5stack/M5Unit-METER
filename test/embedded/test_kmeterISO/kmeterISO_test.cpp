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
using namespace m5::unit::kmeterISO;
using namespace m5::unit::kmeterISO::command;

namespace {
void check_measurement_values(UnitKmeterISO* u)
{
    EXPECT_TRUE(std::isfinite(u->latest().temperature()));
    // M5_LOGI("T:%f", u->latest().temperature());
}

constexpr MeasurementUnit mu_table[] = {MeasurementUnit::Celsius, MeasurementUnit::Fahrenheit};

}  // namespace

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<100000U>());

class TestKmeterISO : public ComponentTestBase<UnitKmeterISO, bool> {
protected:
    virtual UnitKmeterISO* get_instance() override
    {
        auto ptr         = new m5::unit::UnitKmeterISO();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = 8;
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
    for (auto&& mu : mu_table) {
        uint32_t cnt{16};
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

    constexpr uint32_t interval{25};

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& mu : mu_table) {
        std::string s{};
        s = m5::utility::formatString("Munit:%u", m5::stl::to_underlying(mu));
        SCOPED_TRACE(s.c_str());

        EXPECT_TRUE(unit->startPeriodicMeasurement(interval, mu));
        EXPECT_TRUE(unit->inPeriodic());

        test_periodic_measurement(unit.get(), 8, check_measurement_values);

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_EQ(unit->available(), 8U);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        int32_t cnt{0};
        do {
            ++cnt;
            unit->discard();
            EXPECT_FALSE(unit->empty());
            EXPECT_FALSE(unit->full());
            EXPECT_EQ(unit->available(), 8U - cnt);
        } while (cnt < 3);

        unit->flush();
        EXPECT_TRUE(unit->empty());
        EXPECT_FALSE(unit->full());
    }
}

/*
  WARNING!!
  Failure of this test will result in an unexpected I2C address being set!
*/
TEST_P(TestKmeterISO, I2CAddress)
{
    SCOPED_TRACE(ustr);

    Data d{};
    uint8_t ver{}, addr{};

    EXPECT_FALSE(unit->changeI2CAddress(0x07));  // Invalid
    EXPECT_FALSE(unit->changeI2CAddress(0x78));  // Invalid

    // Change to 0x08
    EXPECT_TRUE(unit->changeI2CAddress(0x08));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x08);
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));

    // Change to 0x77
    EXPECT_TRUE(unit->changeI2CAddress(0x77));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x77);
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));

    // Change to 0x52
    EXPECT_TRUE(unit->changeI2CAddress(0x52));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x52);
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));

    // Change to default
    EXPECT_TRUE(unit->changeI2CAddress(UnitKmeterISO::DEFAULT_ADDRESS));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, +UnitKmeterISO::DEFAULT_ADDRESS);
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
    EXPECT_TRUE(unit->measureSingleshot(d));
}
