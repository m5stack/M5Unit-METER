/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Shared test definitions for UnitADS111x (AMeter/VMeter)
  Include AFTER defining: using TestADS1115 = TestAVmeterBase<UnitXxx>;
*/
#include <m5_unit_component/adapter_i2c.hpp>
#include <limits>
#include <utility>

namespace {
inline void check_measurement_values(m5::unit::UnitAVmeterBase* u)
{
    EXPECT_NE(u->adc(), std::numeric_limits<int16_t>::min());
}
}  // namespace

TEST_F(TestADS1115, Address)
{
    SCOPED_TRACE(ustr);
    m5::unit::UnitAVmeterBase tmp(0x00, 0x00);
    EXPECT_FALSE(tmp.begin());
}

TEST_F(TestADS1115, GeneralReset)
{
    SCOPED_TRACE(ustr);

    // I2C_Class hangs on generalReset (bus stuck after general call reset)
    auto ad          = unit->asAdapter<m5::unit::AdapterI2C>(m5::unit::Adapter::Type::I2C);
    bool is_i2cclass = ad && ad->implType() == m5::unit::AdapterI2C::ImplType::I2CClass;
    if (is_i2cclass) {
        M5_LOGW("Skip GeneralReset: I2C_Class does not recover from general call reset");
        GTEST_SKIP();
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    // Rewriting config register
    EXPECT_TRUE(unit->writeMultiplexer(Mux::AIN_23));
    EXPECT_TRUE(unit->writeGain(Gain::PGA_256));
    EXPECT_TRUE(unit->writeSamplingRate(Sampling::Rate475));
    EXPECT_TRUE(unit->writeComparatorQueue(ComparatorQueue::Four));
    EXPECT_TRUE(unit->writeThreshold(0x7123, 0x8123));

    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());

    EXPECT_TRUE(unit->generalReset());

    constexpr uint16_t default_value{0x8583};
    EXPECT_EQ(unit->multiplexer(), Mux::AIN_01);
    EXPECT_EQ(unit->gain(), Gain::PGA_2048);
    EXPECT_EQ(unit->samplingRate(), Sampling::Rate128);
    EXPECT_EQ(unit->comparatorQueue(), ComparatorQueue::Disable);

    uint16_t v{};
    EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, v, 0));
    EXPECT_EQ(v, default_value);

    constexpr int16_t default_high = 0x7FFF;
    constexpr int16_t default_low  = 0x8000;
    int16_t high{}, low{};
    EXPECT_TRUE(unit->readThreshold(high, low));
    EXPECT_EQ(high, default_high);
    EXPECT_EQ(low, default_low);
}

TEST_F(TestADS1115, Configuration)
{
    SCOPED_TRACE(ustr);

    uint16_t prev{}, now{};
    {
        constexpr Mux mux_table[] = {
            Mux::GND_0, Mux::GND_1, Mux::GND_2, Mux::GND_3, Mux::AIN_01, Mux::AIN_03, Mux::AIN_13, Mux::AIN_23,
        };

        SCOPED_TRACE("Mux");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        for (auto&& e : mux_table) {
            EXPECT_TRUE(unit->writeMultiplexer(e));

            EXPECT_EQ(unit->multiplexer(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;
        }
    }

    {
        constexpr Gain gain_table[] = {
            Gain::PGA_6144, Gain::PGA_4096, Gain::PGA_2048, Gain::PGA_1024, Gain::PGA_512, Gain::PGA_256,
        };

        SCOPED_TRACE("Gain");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        auto prev_c = unit->coefficient();
        EXPECT_TRUE(std::isfinite(prev_c));

        for (auto&& e : gain_table) {
            EXPECT_TRUE(unit->writeGain(e));

            EXPECT_EQ(unit->gain(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;

            auto now_c = unit->coefficient();
            EXPECT_TRUE(std::isfinite(now_c));
            EXPECT_NE(now_c, prev_c);
            prev_c = now_c;
        }
    }

    {
        SCOPED_TRACE("Mode");
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        EXPECT_TRUE(unit->startPeriodicMeasurement());
        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
        EXPECT_NE(now, prev);
        prev = now;

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
        EXPECT_NE(now, prev);
    }

    {
        constexpr Sampling rate_table[] = {
            Sampling::Rate8,   Sampling::Rate16,  Sampling::Rate32,  Sampling::Rate64,
            Sampling::Rate128, Sampling::Rate250, Sampling::Rate475, Sampling::Rate860,
        };

        SCOPED_TRACE("Rate");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        for (auto&& e : rate_table) {
            EXPECT_TRUE(unit->writeSamplingRate(e));

            EXPECT_EQ(unit->samplingRate(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;
        }
    }

    constexpr bool bool_table[] = {true, false};
    {
        SCOPED_TRACE("COMP_MODE");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        for (auto&& e : bool_table) {
            EXPECT_TRUE(unit->writeComparatorMode(e));

            EXPECT_EQ(unit->comparatorMode(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;
        }
    }

    {
        SCOPED_TRACE("COMP_POL");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        for (auto&& e : bool_table) {
            EXPECT_TRUE(unit->writeComparatorPolarity(e));

            EXPECT_EQ(unit->comparatorPolarity(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;
        }
    }

    {
        SCOPED_TRACE("COMP_LAT");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        for (auto&& e : bool_table) {
            EXPECT_TRUE(unit->writeLatchingComparator(e));

            EXPECT_EQ(unit->latchingComparator(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;
        }
    }

    {
        constexpr ComparatorQueue que_table[] = {
            ComparatorQueue::One,
            ComparatorQueue::Two,
            ComparatorQueue::Four,
            ComparatorQueue::Disable,
        };

        SCOPED_TRACE("COMP_QUE");
        EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, prev, 0));

        for (auto&& e : que_table) {
            EXPECT_TRUE(unit->writeComparatorQueue(e));

            EXPECT_EQ(unit->comparatorQueue(), e);

            EXPECT_TRUE(unit->readRegister16BE(command::CONFIG_REG, now, 0));
            EXPECT_NE(now, prev);
            prev = now;
        }
    }

    {
        std::pair<int16_t, int16_t> thres_table[] = {{100, -100}, {1000, -1000}, {0x7FFF, 0x8000}};
        for (auto&& e : thres_table) {
            EXPECT_TRUE(unit->writeThreshold(e.first, e.second));
            int16_t high{}, low{};
            EXPECT_TRUE(unit->readThreshold(high, low));
            EXPECT_EQ(high, e.first);
            EXPECT_EQ(low, e.second);
        }
    }
}

TEST_F(TestADS1115, Periodic)
{
    SCOPED_TRACE(ustr);

    std::tuple<const char*, Sampling> table[] = {
        {"8sps", Sampling::Rate8},     {"16sps", Sampling::Rate16},   {"32sps", Sampling::Rate32},
        {"64sps", Sampling::Rate64},   {"128sps", Sampling::Rate128}, {"250sps", Sampling::Rate250},
        {"475sps", Sampling::Rate475}, {"860sps", Sampling::Rate860},
    };

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_EQ(unit->adc(), std::numeric_limits<int16_t>::min());

    {
        // float correction = unit->resolution() * unit->calibrationFactor();
        for (auto&& e : table) {
            const char* s{};
            Sampling rate{};
            std::tie(s, rate) = e;
            SCOPED_TRACE(s);
            EXPECT_TRUE(unit->writeSamplingRate(rate));
            EXPECT_TRUE(unit->startPeriodicMeasurement());
            EXPECT_TRUE(unit->inPeriodic());

            auto r = collect_periodic_measurements(unit.get(), 4, 0, check_measurement_values);
            EXPECT_FALSE(r.timed_out);
            EXPECT_EQ(r.update_count, 4U);
            EXPECT_LE(r.median(), r.expected_interval + 1);

            EXPECT_TRUE(unit->stopPeriodicMeasurement());
            EXPECT_FALSE(unit->inPeriodic());

            EXPECT_TRUE(unit->full());
            EXPECT_FALSE(unit->empty());
            EXPECT_EQ(unit->available(), 4U);
        }
    }
}

TEST_F(TestADS1115, SingleShot)
{
    SCOPED_TRACE(ustr);

    m5::unit::ads111x::Data d{};

    EXPECT_FALSE(unit->measureSingleshot(d));
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    //    float correction = unit->resolution() * unit->calibrationFactor();
    int cnt{16};
    while (cnt--) {
        EXPECT_TRUE(unit->measureSingleshot(d));
        //        M5_LOGI("raw:%d current:%f", raw, raw * correction);
    }
}
