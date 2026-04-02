/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Graphical meter example for Unit-Meter

  The core must be equipped with LCD.
  Ameter, Vmeter, KmeterISO, or DualKmeter must be connected.

  Controls:
    BtnA hold    : Cycle through fixed scale ranges
    BtnA click   : Pause / Resume graph scrolling
    BtnB click   : Switch measurement channel (DualKmeter only)
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

//#define OUTPUT_DEBUG

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
LGFX_Sprite value_sprite;

#if defined(USING_UNIT_VMETER)
#pragma message "Using Vmeter"
m5::unit::UnitVmeter unit{};
constexpr char tag[] = "Vol";
constexpr m5gfx::rgb565_t theme_clr{48, 144, 224};
#elif defined(USING_UNIT_AMETER)
#pragma message "Using Ameter"
m5::unit::UnitAmeter unit{};
constexpr char tag[] = "Cur";
constexpr m5gfx::rgb565_t theme_clr{255, 80, 144};
#elif defined(USING_UNIT_KMETER_ISO)
m5::unit::UnitKmeterISO unit{};
#pragma message "Using KmeterISO"
constexpr char tag[] = "Tmp";
constexpr m5gfx::rgb565_t theme_clr{240, 188, 104};
#elif defined(USING_UNIT_DUAL_KMETER)
m5::unit::UnitDualKmeter unit{0x11};  // Configured address
#pragma message "Using DualKmeter"
const char* tag = "DK1";
constexpr m5gfx::rgb565_t theme_clr{240, 188, 104};
#else
#error "Choose unit"
#endif

constexpr size_t SAMPLE_QUEUE_SIZE{256};
QueueHandle_t sample_queue{};
std::vector<float> plot_values;
std::vector<int16_t> plot_y_cache;  // cached screen Y per column (-1 = none)
size_t plot_head{};                 // circular buffer write position
float latest_value{};
float latest_min_value{};
float latest_max_value{};
float shown_min_value{std::numeric_limits<float>::quiet_NaN()};
float shown_max_value{std::numeric_limits<float>::quiet_NaN()};
std::string shown_latest_text{};
uint32_t ups{};
uint32_t sps{};
int32_t active_scale_min{};
int32_t active_scale_max{1};
bool has_active_scale{};

#if defined(USING_UNIT_VMETER)
// Voltage (mV): ±20 ~ ±130000
constexpr int32_t fixed_ranges[][2] = {{0, 100}, {0, 500}, {0, 1000}, {0, 5000}, {0, 10000}, {0, 130000}};
constexpr const char* range_unit    = "mV";
#elif defined(USING_UNIT_AMETER)
// Current (mA): ±25 ~ ±500
constexpr int32_t fixed_ranges[][2] = {{-25, 25}, {-250, 250}, {-500, 500}};
constexpr const char* range_unit    = "mA";
#elif defined(USING_UNIT_KMETER_ISO) || defined(USING_UNIT_DUAL_KMETER)
// Temperature (C): K-type thermocouple -200~+1350
constexpr int32_t fixed_ranges[][2] = {{-10, 40}, {-50, 50}, {-100, 100}, {-200, 200}, {-200, 500}, {-200, 1350}};
constexpr const char* range_unit    = "C";
#endif
constexpr size_t range_count = sizeof(fixed_ranges) / sizeof(fixed_ranges[0]);
size_t range_idx{0};
bool paused{};

inline void apply_range()
{
    active_scale_min = fixed_ranges[range_idx][0];
    active_scale_max = fixed_ranges[range_idx][1];
    has_active_scale = true;
    latest_min_value = static_cast<float>(active_scale_min);
    latest_max_value = static_cast<float>(active_scale_max);
}

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

inline float plot_at(size_t screen_col)
{
    return plot_values[(plot_head + screen_col) % plot_values.size()];
}

// Compute the vertical pixel range at column sc for a polyline using exact Bresenham split.
// For dx=1 segment: left column gets floor(|dy|/2)+1 pixels, right gets ceil(|dy|/2) pixels.
inline void column_y_range(const int16_t* y, size_t n, size_t sc, int16_t& top, int16_t& bot)
{
    const int16_t cy = y[sc];
    if (cy < 0) {
        top = bot = -1;
        return;
    }
    top = bot = cy;

    // Left segment: (sc-1, prev) → (sc, cy) — sc is the RIGHT column
    // Right column extends ceil(|dy|/2)-1 pixels from cy toward prev
    if (sc > 0 && y[sc - 1] >= 0) {
        const int16_t dy  = static_cast<int16_t>(std::abs(cy - y[sc - 1]));
        const int16_t ext = static_cast<int16_t>((dy + 1) / 2 - 1);  // ceil(dy/2) - 1
        if (y[sc - 1] < cy) {
            const int16_t t = static_cast<int16_t>(cy - ext);
            if (t < top) top = t;
        } else if (y[sc - 1] > cy) {
            const int16_t b = static_cast<int16_t>(cy + ext);
            if (b > bot) bot = b;
        }
    }

    // Right segment: (sc, cy) → (sc+1, next) — sc is the LEFT column
    // Left column extends floor(|dy|/2) pixels from cy toward next
    if (sc + 1 < n && y[sc + 1] >= 0) {
        const int16_t dy  = static_cast<int16_t>(std::abs(y[sc + 1] - cy));
        const int16_t ext = static_cast<int16_t>(dy / 2);  // floor(dy/2)
        if (y[sc + 1] > cy) {
            const int16_t b = static_cast<int16_t>(cy + ext);
            if (b > bot) bot = b;
        } else if (y[sc + 1] < cy) {
            const int16_t t = static_cast<int16_t>(cy - ext);
            if (t < top) top = t;
        }
    }
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

int16_t to_plot_y(const float value)
{
    const float minv            = static_cast<float>(active_scale_min);
    const float maxv            = static_cast<float>(active_scale_max);
    const float range           = std::max(1.0f, maxv - minv);
    const int32_t top_plot_y    = graph_top + 1;
    const int32_t bottom_plot_y = graph_top + graph_height - 1;
    const int32_t plot_span     = std::max<int32_t>(1, bottom_plot_y - top_plot_y);
    const float clamped         = m5::stl::clamp(value, minv, maxv);
    return static_cast<int16_t>(bottom_plot_y - ((clamped - minv) * plot_span / range));
}

void redraw_full_graph()
{
    if (!has_active_scale || graph_height <= 0 || meter_width <= 0) {
        return;
    }
    lcd.fillRect(meter_offset_x, graph_top, meter_width, graph_height, TFT_BLACK);
    draw_gauge_full_width();

    const size_t n = plot_values.size();
    // Rebuild Y cache
    for (size_t sc = 0; sc < n; ++sc) {
        const float v    = plot_at(sc);
        plot_y_cache[sc] = std::isfinite(v) ? to_plot_y(v) : -1;
    }
    // Draw polyline using column ranges
    for (size_t sc = 0; sc < n; ++sc) {
        int16_t top, bot;
        column_y_range(plot_y_cache.data(), n, sc, top, bot);
        if (top >= 0) {
            const int32_t x = meter_offset_x + static_cast<int32_t>(sc);
            lcd.writeFastVLine(x, top, bot - top + 1, theme_clr);
        }
    }
    draw_gauge_full_width();
}

void reset_graph()
{
    std::fill(plot_values.begin(), plot_values.end(), std::numeric_limits<float>::quiet_NaN());
    std::fill(plot_y_cache.begin(), plot_y_cache.end(), int16_t(-1));
    plot_head        = 0;
    latest_value     = 0.0f;
    latest_min_value = 0.0f;
    latest_max_value = 1.0f;
    shown_min_value  = std::numeric_limits<float>::quiet_NaN();
    shown_max_value  = std::numeric_limits<float>::quiet_NaN();
    shown_latest_text.clear();
    has_active_scale = false;
    lcd.fillRect(meter_offset_x, 0, meter_width, plot_height, TFT_BLACK);
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
    auto f  = lcd.getFont();
    auto td = lcd.getTextDatum();

    if (!footer_static_drawn) {
        lcd.fillRect(meter_offset_x, plot_height, meter_width, footer_height, TFT_BLACK);
        lcd.fillRect(meter_offset_x, plot_height, meter_width, 4, theme_clr);
        footer_value_left  = meter_offset_x + meter_width / 4;
        footer_value_right = meter_offset_x + meter_width;
        // Tag label (draw once)
        lcd.setFont(meter_width >= 240 ? &fonts::FreeSansBold12pt7b : &fonts::FreeSansBold9pt7b);
        lcd.setTextDatum(textdatum_t::middle_left);
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        lcd.drawString(tag, meter_offset_x, plot_height + footer_height / 2);
        footer_static_drawn = true;
    }

    lcd.setTextDatum(td);
    lcd.setFont(f);
}

void draw_latest_value()
{
    auto s = m5::utility::formatString("%.2f", latest_value);
    if (s == shown_latest_text) {
        return;
    }
    const int32_t w = footer_value_right - footer_value_left;
    const int32_t h = footer_height - 4;
    value_sprite.fillScreen(TFT_BLACK);
    value_sprite.drawString(s.c_str(), w, h / 2);
    value_sprite.pushSprite(&lcd, footer_value_left, plot_height + 4);
    shown_latest_text = std::move(s);
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

// Bresenham-range diff-redraw: compute Y range per column from adjacent midpoints,
// erase old range, draw new range, restore gauge — all in a single pass.
void redraw_graph_diff()
{
    if (graph_height <= 0 || meter_width <= 0 || !has_active_scale) {
        return;
    }
    const size_t n = plot_values.size();

    // Compute new Y values
    static std::vector<int16_t> new_y;
    new_y.resize(n);
    for (size_t sc = 0; sc < n; ++sc) {
        const float v = plot_at(sc);
        new_y[sc]     = std::isfinite(v) ? to_plot_y(v) : -1;
    }

    // Single-pass diff per column — only touch pixels that actually change
    for (size_t sc = 0; sc < n; ++sc) {
        int16_t ot, ob, nt, nb;
        column_y_range(plot_y_cache.data(), n, sc, ot, ob);
        column_y_range(new_y.data(), n, sc, nt, nb);
        if (ot == nt && ob == nb) {
            continue;
        }
        const int32_t x = meter_offset_x + static_cast<int32_t>(sc);

        if (ot < 0) {
            // No old, just draw new
            lcd.writeFastVLine(x, nt, nb - nt + 1, theme_clr);
            draw_gauge_column(x);
        } else if (nt < 0) {
            // No new, just erase old
            lcd.writeFastVLine(x, ot, ob - ot + 1, TFT_BLACK);
            draw_gauge_column(x);
        } else {
            // Both exist — erase only the non-overlapping old, draw only the non-overlapping new
            // Erase old top above new range
            if (ot < nt) {
                lcd.writeFastVLine(x, ot, std::min<int16_t>(nt, ob + 1) - ot, TFT_BLACK);
            }
            // Erase old bottom below new range
            if (ob > nb) {
                const int16_t erase_start = std::max<int16_t>(nb + 1, ot);
                lcd.writeFastVLine(x, erase_start, ob - erase_start + 1, TFT_BLACK);
            }
            // Draw new top above old range
            if (nt < ot) {
                lcd.writeFastVLine(x, nt, std::min<int16_t>(ot, nb + 1) - nt, theme_clr);
            }
            // Draw new bottom below old range
            if (nb > ob) {
                const int16_t draw_start = std::max<int16_t>(ob + 1, nt);
                lcd.writeFastVLine(x, draw_start, nb - draw_start + 1, theme_clr);
            }
            draw_gauge_column(x);
        }
    }

    // Update cache
    std::copy(new_y.begin(), new_y.end(), plot_y_cache.begin());
}

#if defined(USING_UNIT_DUAL_KMETER)
volatile bool pending_channel_switch{};
volatile bool pending_graph_clear{};
#endif

void update_meter(void*)
{
    for (;;) {
#if defined(USING_UNIT_DUAL_KMETER)
        if (pending_channel_switch) {
            auto ch = (unit.measurementChannel() == m5::unit::dual_kmeter::Channel::One)
                          ? m5::unit::dual_kmeter::Channel::Two
                          : m5::unit::dual_kmeter::Channel::One;
            if (unit.writeCurrentChannel(ch)) {
                tag = (ch == m5::unit::dual_kmeter::Channel::One) ? "DK1" : "DK2";
                M5_LOGI("Channel: %s", tag);
                footer_static_drawn = false;
                pending_graph_clear = true;
            }
            pending_channel_switch = false;
        }
#endif
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

#if defined(USING_UNIT_DUAL_KMETER)
// DualKmeter uses M5-Bus internal I2C
bool initialize_unit(const m5::board_t)
{
    return Units.add(unit, M5.In_I2C) && Units.begin();
}
#else
// Vmeter/Ameter/KmeterISO use GROVE port
bool initialize_unit(const m5::board_t board)
{
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a
        // Wire is used internally, so SoftwareI2C handles the unit.
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        return Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    }
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, unit.component_config().clock);
    return Units.add(unit, Wire) && Units.begin();
}
#endif

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
    M5.setTouchButtonHeightByRatio(100);
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
    plot_y_cache.assign(static_cast<size_t>(meter_width), -1);
    reset_graph();

    apply_range();

    // Unit config
    auto cfg = unit.config();
#if defined(USING_UNIT_VMETER) || defined(USING_UNIT_AMETER)
    cfg.rate = m5::unit::ads111x::Sampling::Rate250;
#elif defined(USING_UNIT_KMETER_ISO) || defined(USING_UNIT_DUAL_KMETER)
    cfg.interval = 20;
#endif
    unit.config(cfg);

    auto ccfg        = unit.component_config();
    ccfg.stored_size = 250;
    unit.component_config(ccfg);

    // I2C initialization
    auto began = initialize_unit(board);
    if (!began) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
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
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("sample queue created");

    draw_footer();

    // Initialize value sprite for flicker-free latest value display
    value_sprite.setPsram(false);
    value_sprite.setColorDepth(lcd.getColorDepth());
    value_sprite.createSprite(footer_value_right - footer_value_left, footer_height - 4);
    value_sprite.setFont(meter_width >= 240 ? &fonts::FreeSansBold12pt7b : &fonts::FreeSansBold9pt7b);
    value_sprite.setTextDatum(textdatum_t::middle_right);
    value_sprite.setTextColor(TFT_WHITE);

    xTaskCreateUniversal(update_meter, "meter", 8192, nullptr, 1, nullptr, PRO_CPU_NUM);
}

void loop()
{
    static uint32_t upsCnt{};
    static uint32_t sampleCnt{};
    static unsigned long start_at{m5::utility::millis()};

    M5.update();

    if (M5.BtnA.wasHold()) {
        range_idx = (range_idx + 1) % range_count;
        apply_range();
        M5_LOGI("Range: %d~%d %s", active_scale_min, active_scale_max, range_unit);
        lcd.startWrite();
        redraw_full_graph();
        lcd.endWrite();
        draw_scale_labels();
        draw_footer();
    }
    if (M5.BtnA.wasClicked()) {
        paused = !paused;
        M5_LOGI("%s", paused ? "Paused" : "Playing");
        draw_footer();
    }
#if defined(USING_UNIT_DUAL_KMETER)
    if (M5.BtnB.wasClicked()) {
        pending_channel_switch = true;
    }
    if (pending_graph_clear) {
        pending_graph_clear = false;
        reset_graph();
        apply_range();
        lcd.startWrite();
        redraw_full_graph();
        lcd.endWrite();
        draw_scale_labels();
        draw_footer();
    }
#endif

    auto now = m5::utility::millis();
    if (now >= start_at + 1000) {
        ups = upsCnt;
        sps = sampleCnt;
#if defined(OUTPUT_DEBUG)
        M5_LOGI("UPS:%u SPS:%u", ups, sps);
#endif
        upsCnt    = 0;
        sampleCnt = 0;
        start_at  = now;
    }

    float v{};
    bool updated{};
    uint32_t samples_this_frame{};

    // Consume all queued samples into circular buffer
    if (paused) {
        while (pop_sample(v)) {
        }  // drain queue, discard
    } else {
        while (pop_sample(v)) {
#if defined(OUTPUT_DEBUG)
            if (plot_head == 0) {
                static unsigned long sweep_start{};
                auto t = m5::utility::millis();
                if (sweep_start) {
                    M5_LOGI("Sweep %u ms (%u cols)", t - sweep_start, plot_values.size());
                }
                sweep_start = t;
            }
#endif
            plot_values[plot_head] = v;
            plot_head              = (plot_head + 1) % plot_values.size();
            latest_value           = v;
            ++samples_this_frame;
        }
    }

    // Diff-redraw: only changed pixels
    if (samples_this_frame > 0) {
        lcd.startWrite();
        redraw_graph_diff();
        lcd.endWrite();
        updated = true;
    }

    if (updated) {
        draw_latest_value();
        draw_scale_labels();
        draw_footer();
    }
    sampleCnt += samples_this_frame;
    if (updated) {
        ++upsCnt;
    }
}
