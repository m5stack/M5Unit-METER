/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Graphical meter example for Unit-Meter

  The core must be equipped with LCD
  Ameter,Vmeter,or KmeterISO unit must be connected
*/
// #define USING_DISPLAY_MODULE

#if defined(USING_DISPLAY_MODULE)
#include <M5ModuleDisplay.h>
#endif
#include <M5Unified.h>
#include <M5GFX.h>
#include <M5HAL.hpp>  // For NessoN1
#include <Wire.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <string>
#include <limits>
#include <freertos/queue.h>
#include <vector>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_VMETER) && !defined(USING_UNIT_AMETER) && !defined(USING_UNIT_KMETER_ISO) && \
    !defined(USING_UNIT_DUAL_KMETER)
// For Vmeter
// #define USING_UNIT_VMETER
// For Ameter
// #define USING_UNIT_AMETER
// For KmeterISO
// #define USING_UNIT_KMETER_ISO
// For DualKmeter
// #define USING_UNIT_DUAL_KMETER
#endif

namespace {
auto& lcd = M5.Display;
int32_t meter_width{};
int32_t meter_height{};
int32_t meter_offset_x{};
int32_t plot_height{};
int32_t footer_height{};
int32_t scale_label_height{};
int32_t graph_top{};
int32_t graph_height{};
int32_t footer_value_left{};
int32_t footer_value_right{};
bool footer_static_drawn{};
m5::unit::UnitUnified Units;

#if defined(USING_UNIT_VMETER)
#pragma message "Using Vmeter"
m5::unit::UnitVmeter unit{};
constexpr char tag[] = "Voltage";
constexpr m5gfx::rgb565_t theme_clr{48, 144, 224};
#elif defined(USING_UNIT_AMETER)
#pragma message "Using Ameter"
m5::unit::UnitAmeter unit{};
constexpr char tag[] = "Current";
constexpr m5gfx::rgb565_t theme_clr{255, 80, 144};
#elif defined(USING_UNIT_KMETER_ISO)
m5::unit::UnitKmeterISO unit{};
#pragma message "Using KmeterISO"
constexpr char tag[] = "Temp";
constexpr m5gfx::rgb565_t theme_clr{240, 188, 104};
#elif defined(USING_UNIT_DUAL_KMETER)
m5::unit::UnitDualKmeter unit{0x11};  // Configured address
#pragma message "Using DualKmeter"
const char* tag = "Temp1";
constexpr m5gfx::rgb565_t theme_clr{240, 188, 104};
#else
#error "Choose unit"
#endif

constexpr size_t SAMPLE_QUEUE_SIZE{256};
QueueHandle_t sample_queue{};
std::vector<float> plot_values;
float latest_value{};
float latest_min_value{};
float latest_max_value{};
float shown_min_value{std::numeric_limits<float>::quiet_NaN()};
float shown_max_value{std::numeric_limits<float>::quiet_NaN()};
std::string shown_latest_text{};
int32_t active_scale_min{};
int32_t active_scale_max{1};
bool has_active_scale{};

inline void push_sample(const float v)
{
    if (!sample_queue) {
        return;
    }
    if (xQueueSend(sample_queue, &v, 0) != pdTRUE) {
        float dummy{};
        xQueueReceive(sample_queue, &dummy, 0);
        xQueueSend(sample_queue, &v, 0);
    }
}

inline bool pop_sample(float& v)
{
    return sample_queue && (xQueueReceive(sample_queue, &v, 0) == pdTRUE);
}

inline const char* current_tag()
{
#if defined(USING_UNIT_DUAL_KMETER)
    return tag;
#else
    return tag;
#endif
}

void draw_gauge_full_width()
{
    if (graph_height <= 0 || meter_width <= 0) {
        return;
    }
    for (int i = 0; i <= 4; ++i) {
        const int32_t gy = graph_top + ((graph_height - 1) * i) / 4;
        lcd.drawFastHLine(meter_offset_x, gy, meter_width, (i == 2) ? TFT_LIGHTGRAY : TFT_DARKGRAY);
    }
}

void draw_gauge_column(const int32_t gx)
{
    if (gx < meter_offset_x || gx >= meter_offset_x + meter_width) {
        return;
    }
    for (int i = 0; i <= 4; ++i) {
        const int32_t gy = graph_top + ((graph_height - 1) * i) / 4;
        lcd.drawPixel(gx, gy, (i == 2) ? TFT_LIGHTGRAY : TFT_DARKGRAY);
    }
}

bool update_quantized_scale_from_history()
{
    float minv{std::numeric_limits<float>::infinity()};
    float maxv{-std::numeric_limits<float>::infinity()};
    for (size_t i = 0; i < plot_values.size(); ++i) {
        const auto v = plot_values[i];
        if (!std::isfinite(v)) {
            continue;
        }
        if (v < minv) {
            minv = v;
        }
        if (v > maxv) {
            maxv = v;
        }
    }
    if (!std::isfinite(minv) || !std::isfinite(maxv)) {
        return false;
    }
    if (minv >= 0.0f && maxv >= 0.0f) {
        minv = 0.0f;
    } else if (minv <= 0.0f && maxv <= 0.0f) {
        maxv = 0.0f;
    }

    int32_t qmin = static_cast<int32_t>(std::floor(minv));
    int32_t qmax = static_cast<int32_t>(std::ceil(maxv));
    if (qmin >= 0 && qmax >= 0) {
        qmin = 0;
    } else if (qmin <= 0 && qmax <= 0) {
        qmax = 0;
    }
    if (qmin == qmax) {
        if (qmin == 0) {
            qmax = 1;
        } else if (qmin > 0) {
            qmin = 0;
        } else {
            qmax = 0;
        }
    }

    latest_min_value   = static_cast<float>(qmin);
    latest_max_value   = static_cast<float>(qmax);
    const bool changed = (!has_active_scale || qmin != active_scale_min || qmax != active_scale_max);
    active_scale_min   = qmin;
    active_scale_max   = qmax;
    has_active_scale   = true;
    return changed;
}

int16_t to_plot_y(const float value)
{
    const float minv            = static_cast<float>(active_scale_min);
    const float maxv            = static_cast<float>(active_scale_max);
    const float range           = std::max(1.0f, maxv - minv);
    const int32_t top_plot_y    = graph_top + 1;
    const int32_t bottom_plot_y = graph_top + graph_height - 1;
    const int32_t plot_span     = std::max<int32_t>(1, bottom_plot_y - top_plot_y);
    return static_cast<int16_t>(bottom_plot_y - ((value - minv) * plot_span / range));
}

void redraw_full_graph()
{
    if (!has_active_scale || graph_height <= 0 || meter_width <= 0) {
        return;
    }
    lcd.fillRect(meter_offset_x, graph_top, meter_width, graph_height, TFT_BLACK);
    draw_gauge_full_width();

    int32_t prev_x{};
    int16_t prev_y{};
    bool has_prev{};
    for (size_t i = 0; i < plot_values.size(); ++i) {
        const auto v = plot_values[i];
        if (!std::isfinite(v)) {
            has_prev = false;
            continue;
        }
        const auto x = meter_offset_x + static_cast<int32_t>(i);
        const auto y = to_plot_y(v);
        if (has_prev) {
            lcd.drawLine(prev_x, prev_y, x, y, theme_clr);
        } else {
            lcd.drawPixel(x, y, theme_clr);
        }
        prev_x   = x;
        prev_y   = y;
        has_prev = true;
    }
}

void reset_graph()
{
    std::fill(plot_values.begin(), plot_values.end(), std::numeric_limits<float>::quiet_NaN());
    latest_value     = 0.0f;
    latest_min_value = 0.0f;
    latest_max_value = 1.0f;
    shown_min_value  = std::numeric_limits<float>::quiet_NaN();
    shown_max_value  = std::numeric_limits<float>::quiet_NaN();
    shown_latest_text.clear();
    has_active_scale = false;
    lcd.fillRect(meter_offset_x, 0, meter_width, plot_height, TFT_BLACK);
    lcd.setScrollRect(meter_offset_x, graph_top, meter_width, graph_height);
    draw_gauge_full_width();
}

int32_t measure_scale_label_height()
{
    auto f = lcd.getFont();
    lcd.setFont(&fonts::Font2);
    const int32_t h = lcd.fontHeight() + 2;
    lcd.setFont(f);
    return h;
}

void draw_footer()
{
    if (footer_static_drawn) {
        return;
    }
    auto f  = lcd.getFont();
    auto td = lcd.getTextDatum();

    lcd.fillRect(meter_offset_x, plot_height, meter_width, footer_height, TFT_BLACK);
    lcd.fillRect(meter_offset_x, plot_height, meter_width, 4, theme_clr);
    lcd.setFont(meter_width >= 240 ? &fonts::FreeSansBold12pt7b : &fonts::FreeSansBold9pt7b);
    lcd.setTextDatum(textdatum_t::middle_left);
    lcd.drawString(current_tag(), meter_offset_x, plot_height + footer_height / 2);
    footer_value_left   = meter_offset_x + meter_width / 2;
    footer_value_right  = meter_offset_x + meter_width;
    footer_static_drawn = true;

    lcd.setTextDatum(td);
    lcd.setFont(f);
}

void draw_latest_value()
{
    auto f  = lcd.getFont();
    auto td = lcd.getTextDatum();
    lcd.setFont(meter_width >= 240 ? &fonts::FreeSansBold12pt7b : &fonts::FreeSansBold9pt7b);
    lcd.setTextDatum(textdatum_t::middle_right);
    auto s = m5::utility::formatString("%.2f", latest_value);
    if (s == shown_latest_text) {
        lcd.setTextDatum(td);
        lcd.setFont(f);
        return;
    }
    lcd.fillRect(footer_value_left, plot_height + 4, footer_value_right - footer_value_left, footer_height - 4,
                 TFT_BLACK);
    lcd.drawString(s.c_str(), footer_value_right, plot_height + footer_height / 2);
    shown_latest_text = std::move(s);
    lcd.setTextDatum(td);
    lcd.setFont(f);
}

void draw_scale_labels()
{
    if (shown_min_value == latest_min_value && shown_max_value == latest_max_value) {
        return;
    }
    auto f  = lcd.getFont();
    auto td = lcd.getTextDatum();
    lcd.setFont(&fonts::Font2);
    lcd.setTextDatum(textdatum_t::top_right);
    lcd.fillRect(meter_offset_x + meter_width - 88, 0, 88, scale_label_height, TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    auto smax = m5::utility::formatString("%0.2f", latest_max_value);
    lcd.drawString(smax.c_str(), meter_offset_x + meter_width - 2, 0);

    lcd.fillRect(meter_offset_x + meter_width - 88, plot_height - scale_label_height, 88, scale_label_height,
                 TFT_BLACK);
    auto smin = m5::utility::formatString("%0.2f", latest_min_value);
    lcd.drawString(smin.c_str(), meter_offset_x + meter_width - 2, plot_height - scale_label_height);
    lcd.setTextDatum(td);
    lcd.setFont(f);
    shown_min_value = latest_min_value;
    shown_max_value = latest_max_value;
}

void draw_sample_direct(const float value, const int32_t x, const float prev_value, const bool has_prev)
{
    if (graph_height <= 0 || meter_width <= 1 || !has_active_scale) {
        return;
    }
    latest_value = value;

    lcd.fillRect(x, graph_top, 1, graph_height, TFT_BLACK);
    draw_gauge_column(x);
    draw_gauge_column(x - 1);

    const auto y = to_plot_y(value);
    if (has_prev && std::isfinite(prev_value)) {
        const auto prev_y = to_plot_y(prev_value);
        lcd.drawLine(x - 1, prev_y, x, y, theme_clr);
    } else {
        lcd.drawPixel(x, y, theme_clr);
    }
}

void update_meter(void*)
{
    for (;;) {
        Units.update();
        if (unit.available()) {
            do {
                float v{};
#if defined(USING_UNIT_VMETER)
                v = unit.voltage();
#elif defined(USING_UNIT_AMETER)
                v = unit.current();
#elif defined(USING_UNIT_KMETER_ISO) || defined(USING_UNIT_DUAL_KMETER)
                v = unit.temperature();
#endif
                push_sample(v);
                unit.discard();
            } while (unit.available());
        }
        m5::utility::delay(1);
    }
}

}  // namespace

void setup()
{
    auto m5cfg = M5.config();
#if defined(__M5GFX_M5MODULEDISPLAY__)
    m5cfg.module_display.logical_width  = 320;
    m5cfg.module_display.logical_height = 240;
#endif

#if defined(USING_UNIT_DUAL_KMETER)
    // Disable because it conflicts with internal i2c
    m5cfg.internal_imu = false;  // Disable internal IMU
    m5cfg.internal_rtc = false;  // Disable internal RTC
#endif
    M5.begin(m5cfg);
#if defined(USING_UNIT_DUAL_KMETER)
    // Keep touch enabled, but disable touch-to-BtnA mapping.
    M5.setTouchButtonHeightByRatio(0);
#else
    M5.setTouchButtonHeightByRatio(100);
#endif
    const auto board = M5.getBoard();

    // No LCD or display device?
    if (lcd.width() == 0 || lcd.isEPD()) {
        M5_LOGE("The core must be equipped with LCD");
        M5.Speaker.tone(1000, 20);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // The screen shall be in landscape mode (except Tab5)
    if (board != m5::board_t::board_M5Tab5 && lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(__M5GFX_M5MODULEDISPLAY__)
    // Choose Display if Display module and cable connected
    int32_t idx = M5.getDisplayIndex(m5gfx::board_M5ModuleDisplay);
    M5_LOGI("ModuleDisplay?:%d", idx);
    if (idx >= 0) {
        uint8_t buf[256];
        if (0 < ((lgfx::Panel_M5HDMI*)(M5.Displays(idx).panel()))->readEDID(buf, sizeof(buf))) {
            M5_LOGI("Detected the display, Set Display primary");
            M5.setPrimaryDisplay(idx);
        }
    }
#endif

    // For Tab5, use full width and cap height at 240
    if (board == m5::board_t::board_M5Tab5) {
        meter_width    = lcd.width();
        meter_height   = lcd.height() < 240 ? lcd.height() : 240;
        meter_offset_x = (lcd.width() - meter_width) / 2;
    } else {
        meter_width    = lcd.width();
        meter_height   = lcd.height();
        meter_offset_x = 0;
    }
    footer_height = meter_height < 32 ? meter_height / 3 : 32;
    if (footer_height < 16) {
        footer_height = 16;
    }
    plot_height = meter_height - footer_height;
    if (plot_height < 8) {
        plot_height = 8;
    }
    scale_label_height                = std::max<int32_t>(0, measure_scale_label_height());
    const int32_t reserved_for_labels = scale_label_height * 2;
    graph_top                         = scale_label_height;
    graph_height                      = plot_height - reserved_for_labels;
    if (graph_height < 8) {
        graph_top    = 0;
        graph_height = plot_height;
    }
    plot_values.assign(static_cast<size_t>(meter_width), std::numeric_limits<float>::quiet_NaN());
    reset_graph();

    // UnitUnified
#if defined(USING_UNIT_DUAL_KMETER)
    auto pin_num_sda = M5.getPin(m5::pin_name_t::in_i2c_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::in_i2c_scl);
#else
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
#endif
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    auto cfg = unit.config();

    // clang-format off
#if defined(USING_UNIT_VMETER) || defined(USING_UNIT_AMETER)
    cfg.rate = m5::unit::ads111x::Sampling::Rate250;
#elif defined(USING_UNIT_KMETER_ISO) || defined(USING_UNIT_DUAL_KMETER)
    cfg.interval = 20;
#endif
    // clang-format on

    unit.config(cfg);

    auto ccfg        = unit.component_config();
    ccfg.stored_size = 250;
    unit.component_config(ccfg);

    bool began{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // Port A of the NessoN1 is QWIIC, then use portB (GROVE).
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(NessoN1): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        // Wire is used internally, so SoftwareI2C handles the unit.
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        began = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
#if defined(USING_UNIT_DUAL_KMETER)
    } else if (board == m5::board_t::board_M5Tab5) {
        // Tab5: internal I2C is handled on Wire1.
        Wire1.end();
        Wire1.begin(pin_num_sda, pin_num_scl, unit.component_config().clock);
        began = Units.add(unit, Wire1) && Units.begin();
#endif
    } else {
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, unit.component_config().clock);
        began = Units.add(unit, Wire) && Units.begin();
    }
    if (!began) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
#if defined(USING_UNIT_DUAL_KMETER)
    if (!unit.writeCurrentChannel(m5::unit::dual_kmeter::Channel::One)) {
        M5_LOGW("Failed to set fixed channel CH0");
    }
#endif

    sample_queue = xQueueCreate(SAMPLE_QUEUE_SIZE, sizeof(float));
    if (!sample_queue) {
        M5_LOGE("Failed to create sample queue");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("sample queue created");

    draw_footer();

#if defined(APP_CPU_NUM)
    constexpr BaseType_t task_core = APP_CPU_NUM;
#else
    constexpr BaseType_t task_core = PRO_CPU_NUM;
#endif
    xTaskCreateUniversal(update_meter, "meter", 8192, nullptr, 1, nullptr, task_core);
}

// #define SHOW_SPS

void loop()
{
#if defined(SHOW_SPS)
    static uint32_t fpsCnt{};
    static uint32_t sampleCnt{}, sps{};
    static float spf{};
    static unsigned long start_at{m5::utility::millis()};

    auto now = m5::utility::millis();
    if (now >= start_at + 1000) {
        sps = sampleCnt;
        spf = fpsCnt ? static_cast<float>(sps) / fpsCnt : 0.0f;
        M5_LOGI("loop SPS:%u SPF:%f", sps, spf);
        fpsCnt    = 0;
        sampleCnt = 0;
        start_at  = now;
    }
#endif

    float v{};
    bool updated{};
    uint32_t samples_this_frame{};
    std::vector<float> frame_samples;
    frame_samples.reserve(static_cast<size_t>(meter_width));
    while (pop_sample(v)) {
        if (frame_samples.size() < static_cast<size_t>(meter_width)) {
            frame_samples.push_back(v);
        } else {
            for (size_t i = 1; i < frame_samples.size(); ++i) {
                frame_samples[i - 1] = frame_samples[i];
            }
            frame_samples.back() = v;
        }
        if (plot_values.size() >= 2) {
            for (size_t i = 0; i + 1 < plot_values.size(); ++i) {
                plot_values[i] = plot_values[i + 1];
            }
            plot_values.back() = v;
        }
        ++samples_this_frame;
    }
    const auto store_count   = static_cast<uint32_t>(frame_samples.size());
    const bool scale_changed = (store_count > 0) ? update_quantized_scale_from_history() : false;
    if (store_count > 0) {
        lcd.startWrite();
        if (scale_changed) {
            redraw_full_graph();
        } else {
            lcd.scroll(-static_cast<int32_t>(store_count), 0);
            int32_t x = meter_offset_x + meter_width - static_cast<int32_t>(store_count);
            float prev_value{};
            bool has_prev{};
            const auto anchor_idx = static_cast<size_t>(meter_width - static_cast<int32_t>(store_count) - 1);
            if (anchor_idx < plot_values.size() && std::isfinite(plot_values[anchor_idx])) {
                prev_value = plot_values[anchor_idx];
                has_prev   = true;
            }
            for (size_t i = 0; i < frame_samples.size(); ++i, ++x) {
                draw_sample_direct(frame_samples[i], x, prev_value, has_prev);
                prev_value = frame_samples[i];
                has_prev   = std::isfinite(prev_value);
            }
        }
        draw_latest_value();
        lcd.endWrite();
        updated = true;
    }
#if defined(SHOW_SPS)
    sampleCnt += samples_this_frame;
    if (updated) {
        ++fpsCnt;
    }
#endif
    if (updated) {
        draw_scale_labels();
    }
}
