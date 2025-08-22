/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for INA226
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_INA226.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <random>

#if !defined(USING_UNIT_INA226_1A) && !defined(USING_UNIT_INA226_10A)
#define USING_UNIT_INA226_10A
#endif

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::ina226;
using namespace m5::unit::ina226::command;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

constexpr uint32_t STORED_SIZE{4};

#if defined(USING_UNIT_INA226_10A)
class TestINA226 : public ComponentTestBase<UnitINA226_10A, bool> {
protected:
    virtual UnitINA226_10A* get_instance() override
    {
        auto ptr         = new m5::unit::UnitINA226_10A();
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
#elif defined(USING_UNIT_INA226_1A)
class TestINA226 : public ComponentTestBase<UnitINA226_1A, bool> {
protected:
    virtual UnitINA226_1A* get_instance() override
    {
        auto ptr         = new m5::unit::UnitINA226_1A();
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
#else
#error "Choose unit"
#endif

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestINA226, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestINA226, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestINA226, ::testing::Values(false));

namespace {

auto rng = std::default_random_engine{};

constexpr Sampling rate_table[] = {
    Sampling::Rate1,   Sampling::Rate4,   Sampling::Rate16,  Sampling::Rate64,
    Sampling::Rate128, Sampling::Rate256, Sampling::Rate512, Sampling::Rate1024,
};

constexpr ConversionTime ct_table[] = {
    ConversionTime::US_140,  ConversionTime::US_204,  ConversionTime::US_332,  ConversionTime::US_588,
    ConversionTime::US_1100, ConversionTime::US_2116, ConversionTime::US_4156, ConversionTime::US_8244,
};

constexpr Alert alert_table[] = {
    Alert::ShuntOver, Alert::ShuntUnder, Alert::BusOver, Alert::BusUnder, Alert::PowerOver, Alert::ConversionReady,

};

constexpr uint8_t mb_table[] = {
    0x01,  // Current
    0x02,  // Bus
    0x07,  // Power, Current, Bus
};

#if 0
| #  | Sampling | ctA      | ctB      |
| -- | -------- | -------- | -------- |
| 1  | Rate1    | US\_140  | US\_588  |
| 2  | Rate4    | US\_204  | US\_332  |
| 3  | Rate16   | US\_332  | US\_204  |
| 4  | Rate64   | US\_588  | US\_1100 |
| 5  | Rate128  | US\_1100 | US\_2116 |
| 6  | Rate256  | US\_2116 | US\_4156 |
| 7  | Rate512  | US\_4156 | US\_8244 |
| 8  | Rate1024 | US\_8244 | US\_140  |
| 9  | Rate1    | US\_204  | US\_1100 |
| 10 | Rate4    | US\_4156 | US\_204  |
| 11 | Rate16   | US\_140  | US\_8244 |
| 12 | Rate64   | US\_2116 | US\_332  |
| 13 | Rate128  | US\_332  | US\_2116 |
| 14 | Rate256  | US\_140  | US\_204  |
| 15 | Rate512  | US\_588  | US\_588  |
| 16 | Rate1024 | US\_8244 | US\_4156 |
| 17 | Rate1    | US\_1100 | US\_1100 |
| 18 | Rate4    | US\_588  | US\_332  |
| 19 | Rate16   | US\_8244 | US\_140  |
| 20 | Rate64   | US\_204  | US\_4156 |
#endif

constexpr std::tuple<Sampling, ConversionTime, ConversionTime> param_table[] = {
    {Sampling::Rate1, ConversionTime::US_140, ConversionTime::US_588},
    {Sampling::Rate4, ConversionTime::US_204, ConversionTime::US_332},
    {Sampling::Rate16, ConversionTime::US_332, ConversionTime::US_204},
    {Sampling::Rate64, ConversionTime::US_588, ConversionTime::US_1100},
    {Sampling::Rate128, ConversionTime::US_1100, ConversionTime::US_2116},
    {Sampling::Rate256, ConversionTime::US_2116, ConversionTime::US_4156},
    {Sampling::Rate512, ConversionTime::US_4156, ConversionTime::US_8244},
    {Sampling::Rate1024, ConversionTime::US_8244, ConversionTime::US_140},
    {Sampling::Rate1, ConversionTime::US_204, ConversionTime::US_1100},
    {Sampling::Rate4, ConversionTime::US_4156, ConversionTime::US_204},
    {Sampling::Rate16, ConversionTime::US_140, ConversionTime::US_8244},
    {Sampling::Rate64, ConversionTime::US_2116, ConversionTime::US_332},
    {Sampling::Rate128, ConversionTime::US_332, ConversionTime::US_2116},
    {Sampling::Rate256, ConversionTime::US_140, ConversionTime::US_204},
    {Sampling::Rate512, ConversionTime::US_588, ConversionTime::US_588},
    {Sampling::Rate1024, ConversionTime::US_8244, ConversionTime::US_4156},
    {Sampling::Rate1, ConversionTime::US_1100, ConversionTime::US_1100},
    {Sampling::Rate4, ConversionTime::US_588, ConversionTime::US_332},
    {Sampling::Rate16, ConversionTime::US_8244, ConversionTime::US_140},
    {Sampling::Rate64, ConversionTime::US_204, ConversionTime::US_4156},

};

}  // namespace

TEST_P(TestINA226, Basic)
{
    SCOPED_TRACE(ustr);

#if defined(USING_UNIT_INA226_1A)
    constexpr float res{0.080f};
    constexpr float maxCur{1.0f};
    constexpr float cur{0.00030518509f};
#elif defined(USING_UNIT_INA226_10A)
    constexpr float res{0.005f};
    constexpr float maxCur{10.0f};
    constexpr float cur{0.00030518509f};
#endif

    EXPECT_FLOAT_EQ(unit->shuntResistor(), res);
    EXPECT_FLOAT_EQ(unit->maximumCurrent(), maxCur);
    EXPECT_FLOAT_EQ(unit->currentLSB(), cur);

    uint16_t cal{};
    EXPECT_TRUE(unit->readCalibration(cal));
    EXPECT_NE(cal, 0U);

    Alert alert{};
    EXPECT_TRUE(unit->readAlert(alert));
    EXPECT_EQ(alert, Alert::None);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->powerDown());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());
}

TEST_P(TestINA226, Settings)
{
    SCOPED_TRACE(ustr);

    {
        Mode mode{};
        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_TRUE(unit->readMode(mode));
        EXPECT_EQ(mode, Mode::ShuntAndBus);
    }

    // Failed in periodic
    {
        for (auto&& r : rate_table) {
            EXPECT_FALSE(unit->writeSamplingRate(r));
        }
        for (auto&& ct : ct_table) {
            EXPECT_FALSE(unit->writeShuntVoltageConversionTime(ct));
            EXPECT_FALSE(unit->writeBusVoltageConversionTime(ct));
        }
        EXPECT_FALSE(unit->writeAlertLimit(0x1234));
        for (auto&& a : alert_table) {
            EXPECT_FALSE(unit->writeAlert(a, 0x1234));
        }
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    for (auto&& r : rate_table) {
        EXPECT_TRUE(unit->writeSamplingRate(r));
        Sampling rr{};
        EXPECT_TRUE(unit->readSamplingRate(rr));
        EXPECT_EQ(rr, r);
    }

    for (auto&& ct : ct_table) {
        EXPECT_TRUE(unit->writeShuntVoltageConversionTime(ct));
        ConversionTime ct2{};
        EXPECT_TRUE(unit->readShuntVoltageConversionTime(ct2));
        EXPECT_EQ(ct2, ct);

        EXPECT_TRUE(unit->writeBusVoltageConversionTime(ct));
        EXPECT_TRUE(unit->readBusVoltageConversionTime(ct2));
        EXPECT_EQ(ct2, ct);
    }

    uint16_t lim{};
    EXPECT_TRUE(unit->writeAlertLimit(0x1234));
    EXPECT_TRUE(unit->readAlertLimit(lim));
    EXPECT_EQ(lim, 0x1234);

    for (auto&& a : alert_table) {
        uint16_t v = rng() & 0xFFFF;
        EXPECT_TRUE(unit->writeAlert(a, v));

        Alert a2{};
        EXPECT_TRUE(unit->readAlert(a2));
        EXPECT_EQ(a2, a);
        EXPECT_TRUE(unit->readAlertLimit(lim));
        EXPECT_EQ(lim, v);
    }

    // Reset
    {
        EXPECT_TRUE(unit->softReset());
        Mode m{};
        EXPECT_TRUE(unit->readMode(m));
        EXPECT_EQ(m, Mode::ShuntAndBus);

        Sampling rr{};
        EXPECT_TRUE(unit->readSamplingRate(rr));
        EXPECT_EQ(rr, Sampling::Rate1);

        ConversionTime ct{};
        EXPECT_TRUE(unit->readShuntVoltageConversionTime(ct));
        EXPECT_EQ(ct, ConversionTime::US_1100);
        EXPECT_TRUE(unit->readBusVoltageConversionTime(ct));
        EXPECT_EQ(ct, ConversionTime::US_1100);

        EXPECT_TRUE(unit->readAlertLimit(lim));
        EXPECT_EQ(lim, 0U);
        Alert a{};
        EXPECT_TRUE(unit->readAlert(a));
        EXPECT_EQ(a, Alert::None);
    }
}

#if 0
TEST_P(TestINA226, Singleshot)
{
    SCOPED_TRACE(ustr);
    Data d{};

    // Failed in peiordic
    EXPECT_TRUE(unit->inPeriodic());
    for (auto&& mb : mb_table) {
        EXPECT_FALSE(unit->measureSingleshot(d, mb & 1, mb & 2, mb & 4));
    }

    //
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& r : rate_table) {
        for (auto&& ct : ct_table) {
            for (auto&& mb : mb_table) {
                auto s = m5::utility::formatString("R:%u CT:%u MB:%02X", r, ct, mb);
                SCOPED_TRACE(s);
                EXPECT_TRUE(unit->measureSingleshot(d, r, ct, ct, mb & 1, mb & 2, mb & 4));
            }
        }
    }
}

TEST_P(TestINA226, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& r : rate_table) {
        for (auto&& ct : ct_table) {
            for (auto&& mb : mb_table) {
                auto s = m5::utility::formatString("R:%u CT:%u MB:%02X", r, ct, mb);
                SCOPED_TRACE(s);

                EXPECT_TRUE(unit->startPeriodicMeasurement(r, ct, ct, mb & 1, mb & 2, mb & 4));
                EXPECT_TRUE(unit->inPeriodic());

                test_periodic_measurement(unit.get(), STORED_SIZE, 1);
                EXPECT_TRUE(unit->stopPeriodicMeasurement());
                EXPECT_FALSE(unit->inPeriodic());

                EXPECT_EQ(unit->available(), STORED_SIZE);
                EXPECT_FALSE(unit->empty());
                EXPECT_TRUE(unit->full());

                uint32_t cnt{STORED_SIZE / 2};
                while (cnt-- && unit->available()) {
                    switch (mb) {
                        case 0x01:
                            EXPECT_TRUE(std::isfinite(unit->shuntVoltage()));
                            EXPECT_TRUE(std::isfinite(unit->voltage()));
                            EXPECT_TRUE(std::isfinite(unit->power()));
                            EXPECT_TRUE(std::isfinite(unit->current()));
                            break;
                        case 0x02:
                            EXPECT_TRUE(std::isfinite(unit->shuntVoltage()));
                            EXPECT_TRUE(std::isfinite(unit->voltage()));
                            EXPECT_TRUE(std::isfinite(unit->power()));
                            EXPECT_TRUE(std::isfinite(unit->current()));
                            break;
                        case 0x07:
                            EXPECT_TRUE(std::isfinite(unit->shuntVoltage()));
                            EXPECT_TRUE(std::isfinite(unit->voltage()));
                            EXPECT_TRUE(std::isfinite(unit->power()));
                            EXPECT_TRUE(std::isfinite(unit->current()));
                            break;
                    }

                    EXPECT_FLOAT_EQ(unit->shuntVoltage(), unit->oldest().shuntVoltage());

                    EXPECT_FALSE(unit->empty());
                    unit->discard();
                }
                EXPECT_EQ(unit->available(), STORED_SIZE / 2);
                EXPECT_FALSE(unit->empty());
                EXPECT_FALSE(unit->full());

                unit->flush();
                EXPECT_EQ(unit->available(), 0);
                EXPECT_TRUE(unit->empty());
                EXPECT_FALSE(unit->full());

                EXPECT_FALSE(std::isfinite(unit->shuntVoltage()));
                EXPECT_FALSE(std::isfinite(unit->voltage()));
                EXPECT_FALSE(std::isfinite(unit->power()));
                EXPECT_FALSE(std::isfinite(unit->current()));
            }
        }
    }
}
#else

TEST_P(TestINA226, Singleshot)
{
    SCOPED_TRACE(ustr);
    Data d{};

    // Failed in peiordic
    EXPECT_TRUE(unit->inPeriodic());
    for (auto&& mb : mb_table) {
        EXPECT_FALSE(unit->measureSingleshot(d, mb & 1, mb & 2, mb & 4));
    }

    //
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& p : param_table) {
        for (auto&& mb : mb_table) {
            Sampling r{};
            ConversionTime ct1{}, ct2{};
            std::tie(r, ct1, ct2) = p;

            auto s = m5::utility::formatString("R:%u CT:%u/%u MB:%02X", r, ct1, ct2, mb);
            SCOPED_TRACE(s);
            M5_LOGI("SS:[%s]", s.c_str());

            Data d{};
            EXPECT_TRUE(unit->measureSingleshot(d, r, ct1, ct2, mb & 1, mb & 2, mb & 4));
        }
    }
}

TEST_P(TestINA226, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& p : param_table) {
        for (auto&& mb : mb_table) {
            Sampling r{};
            ConversionTime ct1{}, ct2{};
            std::tie(r, ct1, ct2) = p;

            auto s = m5::utility::formatString("R:%u CT:%u/%u MB:%02X", r, ct1, ct2, mb);
            SCOPED_TRACE(s);
            M5_LOGI("MP:[%s]", s.c_str());

            EXPECT_TRUE(unit->startPeriodicMeasurement(r, ct1, ct2, mb & 1, mb & 2, mb & 4));
            EXPECT_TRUE(unit->inPeriodic());

            test_periodic_measurement(unit.get(), STORED_SIZE, 1);
            EXPECT_TRUE(unit->stopPeriodicMeasurement());
            EXPECT_FALSE(unit->inPeriodic());

            EXPECT_EQ(unit->available(), STORED_SIZE);
            EXPECT_FALSE(unit->empty());
            EXPECT_TRUE(unit->full());

            uint32_t cnt{STORED_SIZE / 2};
            while (cnt-- && unit->available()) {
                switch (mb) {
                    case 0x01:
                        EXPECT_TRUE(std::isfinite(unit->shuntVoltage()));
                        EXPECT_TRUE(std::isfinite(unit->voltage()));
                        EXPECT_TRUE(std::isfinite(unit->power()));
                        EXPECT_TRUE(std::isfinite(unit->current()));
                        break;
                    case 0x02:
                        EXPECT_TRUE(std::isfinite(unit->shuntVoltage()));
                        EXPECT_TRUE(std::isfinite(unit->voltage()));
                        EXPECT_TRUE(std::isfinite(unit->power()));
                        EXPECT_TRUE(std::isfinite(unit->current()));
                        break;
                    default:
                        EXPECT_TRUE(std::isfinite(unit->shuntVoltage()));
                        EXPECT_TRUE(std::isfinite(unit->voltage()));
                        EXPECT_TRUE(std::isfinite(unit->power()));
                        EXPECT_TRUE(std::isfinite(unit->current()));
                        break;
                }

                EXPECT_FLOAT_EQ(unit->shuntVoltage(), unit->oldest().shuntVoltage());
                EXPECT_FLOAT_EQ(unit->voltage(), unit->oldest().voltage());
                EXPECT_FLOAT_EQ(unit->power(), unit->oldest().power());
                EXPECT_FLOAT_EQ(unit->current(), unit->oldest().current());

                EXPECT_FALSE(unit->empty());
                unit->discard();
            }
            EXPECT_EQ(unit->available(), STORED_SIZE / 2);
            EXPECT_FALSE(unit->empty());
            EXPECT_FALSE(unit->full());

            unit->flush();
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());

            EXPECT_FALSE(std::isfinite(unit->shuntVoltage()));
            EXPECT_FALSE(std::isfinite(unit->voltage()));
            EXPECT_FALSE(std::isfinite(unit->power()));
            EXPECT_FALSE(std::isfinite(unit->current()));
        }
    }
}

#endif
