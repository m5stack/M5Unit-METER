/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Base fixture for UnitADS111x (AMeter/VMeter shared)
*/
#ifndef AVMETER_BASE_HPP
#define AVMETER_BASE_HPP

#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_ADS1115.hpp>
#include <unit/unit_Vmeter.hpp>

using namespace m5::unit;
using namespace m5::unit::googletest;
using namespace m5::unit::ads111x;

template <class UnitType>
class TestAVmeterBase : public I2CComponentTestBase<UnitAVmeterBase> {
protected:
    virtual UnitAVmeterBase* get_instance() override
    {
        auto ptr = new UnitType();
        if (ptr) {
            auto ccfg        = ptr->component_config();
            ccfg.stored_size = 4;
            ptr->component_config(ccfg);
        }
        return ptr;
    }
};

#endif  // AVMETER_BASE_HPP
