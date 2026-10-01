# ESP-IDF component manifest for M5Unit-METER.
# Upstream: https://github.com/m5stack/M5Unit-METER
#
# Ameter / Vmeter (ADS111x) / KmeterISO / DualKmeter / INA226-10A / INA226-1A over I2C.
# The public headers include M5UnitUnified and M5Utility (m5_utility/container/circular_buffer.hpp,
# m5_utility/stl/extension.hpp), so both are public requirements.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility
)
