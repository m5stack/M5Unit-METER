/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitAmeter
*/
#include "../avmeter_base.hpp"
#include <unit/unit_Ameter.hpp>

using TestADS1115 = TestAVmeterBase<UnitAmeter>;

#include "../avmeter_template.hpp"

// For UnitAmeter-specific testing
class TestAmeter : public I2CComponentTestBase<UnitAmeter> {
protected:
    virtual UnitAmeter* get_instance() override
    {
        auto ptr = new m5::unit::UnitAmeter();
        if (ptr) {
            auto cfg = ptr->config();
            ptr->config(cfg);
        }
        return ptr;
    }
};

TEST_F(TestAmeter, Correction)
{
    SCOPED_TRACE(ustr);

    constexpr Gain gain_table[] = {
        Gain::PGA_6144, Gain::PGA_4096, Gain::PGA_2048, Gain::PGA_1024, Gain::PGA_512, Gain::PGA_256,
    };

    float prev = unit->correction();
    EXPECT_TRUE(std::isfinite(prev));

    for (auto&& e : gain_table) {
        EXPECT_TRUE(unit->writeGain(e));
        auto now = unit->correction();

        EXPECT_TRUE(std::isfinite(now));
        EXPECT_NE(now, prev);
        prev = now;
    }
}
