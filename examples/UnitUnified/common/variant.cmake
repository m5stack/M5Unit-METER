# Map the Kconfig "Target unit" choice (common/Kconfig.variant.*) to the source-level macro that
# both the Arduino and ESP-IDF builds use. Include this from an example's main/CMakeLists.txt
# *after* idf_component_register() (it needs ${COMPONENT_LIB}). Only the examples that have a
# variant (UnitINA226/PlotToSerial, GraphicalMeter) include this file; each of them rsource's
# exactly one Kconfig.variant.*, so only one of the choices below is ever defined.
set(M5UNIT_VARIANT "")
# UnitINA226/PlotToSerial (common/Kconfig.variant.ina226)
if(CONFIG_EXAMPLE_USING_UNIT_INA226_10A)
    set(M5UNIT_VARIANT USING_UNIT_INA226_10A)
elseif(CONFIG_EXAMPLE_USING_UNIT_INA226_1A)
    set(M5UNIT_VARIANT USING_UNIT_INA226_1A)
elseif(CONFIG_EXAMPLE_BUILTIN_UNIT_INA226_10A)
    set(M5UNIT_VARIANT BUILTIN_UNIT_INA226_10A)
# GraphicalMeter (common/Kconfig.variant.meter)
elseif(CONFIG_EXAMPLE_USING_UNIT_VMETER)
    set(M5UNIT_VARIANT USING_UNIT_VMETER)
elseif(CONFIG_EXAMPLE_USING_UNIT_AMETER)
    set(M5UNIT_VARIANT USING_UNIT_AMETER)
elseif(CONFIG_EXAMPLE_USING_UNIT_KMETER_ISO)
    set(M5UNIT_VARIANT USING_UNIT_KMETER_ISO)
elseif(CONFIG_EXAMPLE_USING_UNIT_DUAL_KMETER)
    set(M5UNIT_VARIANT USING_UNIT_DUAL_KMETER)
endif()
if(M5UNIT_VARIANT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${M5UNIT_VARIANT})
endif()
