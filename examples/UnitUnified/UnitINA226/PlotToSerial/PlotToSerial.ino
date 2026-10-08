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
#if !defined(USING_UNIT_INA226_1A) && !defined(USING_UNIT_INA226_10A) && !defined(BUILTIN_UNIT_INA226_10A)
// For UnitINA226-1A (U200-1A)
// #define USING_UNIT_INA226_1A
// For UnitINA226-10A (U200)
// #define USING_UNIT_INA226_10A
// For Tab5 built-in INA226 (10A)
// #define BUILTIN_UNIT_INA226_10A
#endif
#include "main/PlotToSerial.cpp"
