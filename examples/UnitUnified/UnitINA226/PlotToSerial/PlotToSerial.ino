/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitINA226
*/
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_INA226_1A) && !defined(USING_UNIT_INA226_10A) && !defined(USING_UNIT_INA226_10A_IN_TAB5)
// #define USING_UNIT_INA226_1A
// #define USING_UNIT_INA226_10A
// #define USING_UNIT_INA226_10A_IN_TAB5
#endif
#include "main/PlotToSerial.cpp"
