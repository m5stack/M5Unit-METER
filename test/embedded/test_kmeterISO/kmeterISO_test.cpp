/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for KMeterISO
  NOTE: A thermocouple must be connected to the unit for tests to pass.
        Without it, the STM32 never reports data-ready and all measurements time out.
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_KmeterISO.hpp>
#include <chrono>
#include <iostream>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::kmeter_iso;
using namespace m5::unit::kmeter_iso::command;
using m5::unit::types::elapsed_time_t;

constexpr uint32_t STORED_SIZE{6};

class TestKmeterISO : public I2CComponentTestBase<UnitKmeterISO> {
protected:
    virtual UnitKmeterISO* get_instance() override
    {
        auto ptr         = new m5::unit::UnitKmeterISO();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
};

namespace {

constexpr MeasurementUnit mu_table[] = {MeasurementUnit::Celsius, MeasurementUnit::Fahrenheit};
constexpr uint32_t interval_table[]  = {20, 100, 500};

}  // namespace

TEST_F(TestKmeterISO, Basic)
{
    SCOPED_TRACE(ustr);

    // Version
    uint8_t ver{0x00};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);

    // Status
    uint8_t status{};
    EXPECT_TRUE(unit->readStatus(status));

    // Properties
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Celsius);

    unit->setMeasurementUnit(MeasurementUnit::Fahrenheit);
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Fahrenheit);
    unit->setMeasurementUnit(MeasurementUnit::Celsius);
    EXPECT_EQ(unit->measurementUnit(), MeasurementUnit::Celsius);
}

TEST_F(TestKmeterISO, Singleshot)
{
    SCOPED_TRACE(ustr);
    Data d{};

    // Failed in periodic
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

TEST_F(TestKmeterISO, Periodic)
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
TEST_F(TestKmeterISO, I2CAddress)
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
