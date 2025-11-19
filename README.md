# M5Unit - METER

## Overview

Library for Meters using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U086

Unit AMeter is a current meter that can monitor current in real-time. 

It uses a 16-bit ADC digital-to-analog converter ADS1115 internally and communicates via I2C (0X48). 

To ensure measurement accuracy, it has a built-in DC-DC isolated power supply, and the I2C interface is electrically isolated using a low-power isolator CA-IS3020S to prevent noise and surges on the data bus or other circuits from entering the local ground and interfering with or damaging sensitive circuits. 

Each unit is individually calibrated at the factory, with an accuracy of 1% of full scale, ±1 digit reading, a resolution of 0.3mA, and a maximum measurement current of ±4A.

It has an internal 4A fuse to prevent circuit damage from excessive measurement current.

### SKU:U087

Voltmeter Unit is a voltage meter that can monitor the voltage in real time. The 16-bit ADC (analog-to-digital) converter ADS1115 is used internally to communicate through I2C (0X49).

In order to ensure the measurement accuracy, there is a built-in DC-DC isolated power supply, and the I2C interface is also electrically isolated through the low-power isolator CA-IS3020S.

This prevents noise and surges on the data bus or other circuits from entering the local ground terminal to interfere or damage sensitive circuits. Each Unit is individually calibrated when leaving the factory, initial accuracy of 0.1%FS, ±1 count, and a maximum measurement voltage of ±36V.

### SKU:U133-V11

KMeterISO unitis an integrated K-type thermocouple sensor unit that integrates the functions of "acquisition + isolation + communication", using STM32F030+MAX31855KASA 14bit thermocouple digital conversion chip scheme to achieve high-precision temperature acquisition and conversion, MCU using STM32F030 to realize data acquisition and I2C communication interface, 

using CA-IS3641HW as a signal isolator. The unit supports access to thermocouple probes with a measurement range of -200°C to 1350°C, and adopts a universal standard K-type flat interface, which is convenient for subsequent replacement of different measuring probes to match different needs. 

This module is widely used in application scenarios such as temperature collection, control, and monitoring in industrial automation, instrumentation, power and electrical, heat treatment and other fields.

### SKU:M127

The DualKmeter module13.2 is a dual-channel K-type temperature measurement module based on the MAX31855KASA+stm32f030f4p6+galvanic isolation. 

The module has a built-in two-way K-type thermocouple sensor interface, which uses the signal relay to measure the temperature value of the two channels in turn, supporting a measurement range of -200°C to 1350°C, and a measurement accuracy of °C.

At the same time, the module also has built-in voltage and signal isolation chips such as B0505LS-1WR2 and CA-IS3020S, ensuring the 'stability and safety' of the system. 

In addition, the module has a built-in dial code on off, which can easily switch different I2C addresses to meet the different application needs of users. It can be applied to multiple scenarios such as industrial automation and instrument detection.

### SKU:U200

Unit INA226-10A is a fully isolated high-precision current and voltage measurement unit, suitable for scenarios measuring DC 0 ~ 30V voltage and up to 10A current, supporting simultaneous voltage and current measurements.

### SKU:U200-1A

Unit INA226-1A is a fully isolated high-precision current, voltage, and power measurement unit, suitable for scenarios measuring DC 0 ~ 30V voltage and up to 1A current, supporting simultaneous voltage and current measurements.

## Related Link
See also examples using conventional methods here.

- [Unit Ameter & Datasheet](https://docs.m5stack.com/en/unit/ameter)
- [Unit Vmeter & Datasheet](https://docs.m5stack.com/en/unit/vmeter)
- [Unit KMeterISO & Datasheet](https://docs.m5stack.com/en/unit/KMeterISO%20Unit)
- [Unit DualKmeter & Datasheet](https://docs.m5stack.switch-science.com/en/module/DualKmeter%20Module13.2)
- [Unit INA226-10A & Datasheet](https://docs.m5stack.com/en/unit/Unit_INA226-10A)
- [Unit INA226-1A & Datasheet](https://docs.m5stack.com/en/unit/Unit_INA226-1A)


## Required Libraries:

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-METER - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-METER/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)

