/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Graphical meter example for Unit-AnyMeter
  The core must be equipped with LCD
*/
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_VMETER) && !defined(USING_UNIT_AMETER) && !defined(USING_UNIT_KMETER_ISO)
// For Vmeter
// #define USING_UNIT_VMETER
// For Ameter
// #define USING_UNIT_AMETER
// For KmeterISO
// #define USING_UNIT_KMETER_ISO
#endif
#include "main/GraphicalMeter.cpp"
