/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitVmeter
*/
#include "../avmeter_base.hpp"
#include <unit/unit_Vmeter.hpp>

using TestADS1115 = TestAVmeterBase<UnitVmeter>;

#include "../avmeter_template.hpp"

// For UnitVmeter-specific testing
class TestVmeter : public I2CComponentTestBase<UnitVmeter> {
protected:
    virtual UnitVmeter* get_instance() override
    {
        auto ptr = new m5::unit::UnitVmeter();
        if (ptr) {
            auto cfg = ptr->config();
            ptr->config(cfg);
        }
        return ptr;
    }
};

TEST_F(TestVmeter, Correction)
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
