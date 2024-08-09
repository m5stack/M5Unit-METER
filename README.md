# M5Unit - METER

## Overview

Library for AMeter/VMeter using M5UnitUnified.  

The M5UnitUnified version of the library is located under [src/unit](src/unit).  
M5UnitUnfied has a unified API and can control multiple units via PaHub, etc.


### SKU:U086 & SKU:U087

Ameter(SKU:U086) Unit is a current meter that can monitor the current in real time. The 16-bit ADS1115 ADC (analog-to-digital) converter can be used to communicate through I2C protocol (By default the I2C address is 0X48 unless manually modified).

In order to ensure the measurement accuracy, there is a built-in DC-DC isolated power supply.

The I2C interface is also electrically isolated through the low-power isolator module CA-IS3020S, This prevents noise and surges on the data bus or other circuits from entering the local ground terminal to interfere or damage sensitive circuits.

Each Unit is factory calibrated with initial accuracy of 0.1%FS, ±1 count and resolution of 0.3mA.

The unit has a maximum measurement current of ±4A, and an internal integrated 4A fuse to prevent excessive measurement current from burning out the circuit.


Voltmeter(SKU:U087) Unit is a voltage meter that can monitor the voltage in real time. The 16-bit ADC (analog-to-digital) converter ADS1115 is used internally to communicate through I2C (0X49).

In order to ensure the measurement accuracy, there is a built-in DC-DC isolated power supply, and the I2C interface is also electrically isolated through the low-power isolator CA-IS3020S.

This prevents noise and surges on the data bus or other circuits from entering the local ground terminal to interfere or damage sensitive circuits. Each Unit is individually calibrated when leaving the factory, initial accuracy of 0.1%FS, ±1 count, and a maximum measurement voltage of ±36V.

## Related Link

- [Unit Ameter & Datasheet](https://docs.m5stack.com/en/unit/ameter)
- [Unit Vmeter & Datasheet](https://docs.m5stack.com/en/unit/vmeter)

## Required Libraries:

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-METER- MIT](LICENSE)


## Examples
See also [examples/UnitUnified](examples/UnitUnified)

## Doxygen document
If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxyegn](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)
