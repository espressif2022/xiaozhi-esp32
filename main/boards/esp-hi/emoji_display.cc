#include "emoji_display.h"

#include <cstring>
#include <memory>
#include <unordered_map>
#include <tuple>
#include <esp_log.h>
#include <esp_lcd_panel_io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <time.h>

#include "display/lcd_display.h"
#include "mmap_generate_emoji.h"
#include "config.h"

//map 优先 gfx.h 中的 lvgl.h， 但是 在头上有 lvgl.h 的定义， 内部使用 lvgl.h 的定义
//没发解决.h 同时包含的问题
#include "gfx.h"

namespace anim {

static const char* TAG = "emoji";

// LV_FONT_DECLARE(font_puhui_16_4);
extern "C" {
    extern const lv_font_t font_puhui_16_4;
}

// UI element management
static gfx_obj_t* obj_label_tips = nullptr;
static gfx_obj_t* obj_label_time = nullptr;
static gfx_obj_t* obj_anim_eye = nullptr;
static gfx_obj_t* obj_anim_mic = nullptr;
static gfx_obj_t* obj_img_icon = nullptr;
static gfx_image_dsc_t icon_img_dsc;

// Track current icon to determine when to show time
static int current_icon_type = MMAP_EMOJI_ICON_BATTERY_50_BIN;

enum class UIDisplayMode : uint8_t {
    SHOW_ANIM_TOP = 1,  // Show obj_anim_mic
    SHOW_TIME = 2,      // Show obj_label_time
    SHOW_TIPS = 3       // Show obj_label_tips
};

// Function to manage mutual exclusion of UI controls
static void SetUIDisplayMode(UIDisplayMode mode)
{
    // Hide all controls first
    gfx_obj_set_visible(obj_anim_mic, false);
    gfx_obj_set_visible(obj_label_time, false);
    gfx_obj_set_visible(obj_label_tips, false);

    // Show the selected control
    switch (mode) {
    case UIDisplayMode::SHOW_ANIM_TOP:
        gfx_obj_set_visible(obj_anim_mic, true);
        break;
    case UIDisplayMode::SHOW_TIME:
        gfx_obj_set_visible(obj_label_time, true);
        break;
    case UIDisplayMode::SHOW_TIPS:
        gfx_obj_set_visible(obj_label_tips, true);
        break;
    }
}

static void clock_tm_callback(void* user_data)
{
    // Only display time when battery icon is shown
    if (current_icon_type == MMAP_EMOJI_ICON_BATTERY_50_BIN) {
        time_t now;
        struct tm timeinfo;
        time(&now);

        setenv("TZ", "GMT+0", 1);
        tzset();
        localtime_r(&now, &timeinfo);

        char time_str[6];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

        gfx_label_set_text(obj_label_time, time_str);
        SetUIDisplayMode(UIDisplayMode::SHOW_TIME);
    }
}

// Static helper functions for initialization
static void InitializeAssets(mmap_assets_handle_t* assets_handle)
{
    const mmap_assets_config_t assets_cfg = {
        .partition_label = "assets_A",
        .max_files = MMAP_EMOJI_FILES,
        .checksum = MMAP_EMOJI_CHECKSUM,
        .flags = {.mmap_enable = true, .full_check = true}
    };

    mmap_assets_new(&assets_cfg, assets_handle);
}

static void InitializeGraphics(esp_lcd_panel_handle_t panel, gfx_handle_t* engine_handle)
{
    gfx_core_config_t gfx_cfg = {
        .flush_cb = EmojiEngine::OnFlush,
        .update_cb = EmojiEngine::OnUpdate,
        .user_data = panel,
        .flags = {
            .swap = true,
            .double_buffer = true,
            .buff_dma = true,
        },
        .h_res = DISPLAY_WIDTH,
        .v_res = DISPLAY_HEIGHT,
        .fps = 25,
        .buffers = {
            .buf1 = nullptr,
            .buf2 = nullptr,
            // .buf_pixels = DISPLAY_WIDTH * 16,
            .buf_pixels = DISPLAY_WIDTH * 4,
        },
        .task = GFX_EMOTE_INIT_CONFIG()
    };

    gfx_cfg.task.task_stack_caps = MALLOC_CAP_DEFAULT;
    gfx_cfg.task.task_affinity = 0;
    gfx_cfg.task.task_priority = 5;
    gfx_cfg.task.task_stack = 6 * 1024;

    *engine_handle = gfx_emote_init(&gfx_cfg);
}

static void InitializeEyeAnimation(gfx_handle_t engine_handle, mmap_assets_handle_t assets_handle)
{
    obj_anim_eye = gfx_anim_create(engine_handle);

    const void* anim_data = mmap_assets_get_mem(assets_handle, MMAP_EMOJI_IDLE_ONE_32_AAF);
    size_t anim_size = mmap_assets_get_size(assets_handle, MMAP_EMOJI_IDLE_ONE_32_AAF);

    gfx_anim_set_src(obj_anim_eye, anim_data, anim_size);

    gfx_obj_align(obj_anim_eye, GFX_ALIGN_LEFT_MID, 10, 5);
    gfx_anim_set_mirror(obj_anim_eye, true, (DISPLAY_WIDTH - (55 + 10) * 2));
    gfx_anim_set_segment(obj_anim_eye, 0, 0xFFFF, 40, true);
    gfx_anim_start(obj_anim_eye);
}

static void InitializeLabels(gfx_handle_t engine_handle)
{
    // Initialize tips label
    obj_label_tips = gfx_label_create(engine_handle);
    gfx_obj_align(obj_label_tips, GFX_ALIGN_TOP_MID, 20, 5);
    gfx_obj_set_size(obj_label_tips, 130, 20);
    gfx_label_set_text(obj_label_tips, "启动中...");

    ESP_LOGW(TAG, "font_puhui_16_4: %p", &font_puhui_16_4);
    ESP_LOGW(TAG, "font_awesome_16_4 size: %d", sizeof(font_puhui_16_4));
    ESP_LOGW(TAG, "line_height: %d", (int)font_puhui_16_4.line_height);
    ESP_LOGW(TAG, "base_line: %d", (int)font_puhui_16_4.base_line);
    ESP_LOGW(TAG, "subpx: %d", (int)font_puhui_16_4.subpx);
    ESP_LOGW(TAG, "underline_position: %d", (int)font_puhui_16_4.underline_position);
    ESP_LOGW(TAG, "underline_thickness: %d", (int)font_puhui_16_4.underline_thickness);
    ESP_LOGW(TAG, "dsc: %p", font_puhui_16_4.dsc);

    gfx_label_set_font(obj_label_tips, (gfx_font_t)&font_puhui_16_4);
    gfx_label_set_color(obj_label_tips, GFX_COLOR_HEX(0xFFFFFF));
    gfx_label_set_text_align(obj_label_tips, GFX_TEXT_ALIGN_LEFT);
    gfx_label_set_long_mode(obj_label_tips, GFX_LABEL_LONG_SCROLL);
    gfx_label_set_scroll_speed(obj_label_tips, 20);
    gfx_label_set_scroll_loop(obj_label_tips, true);

    // Initialize time label
    obj_label_time = gfx_label_create(engine_handle);
    gfx_obj_align(obj_label_time, GFX_ALIGN_TOP_MID, 0, 5);
    gfx_obj_set_size(obj_label_time, 50, 20);
    gfx_label_set_text(obj_label_time, "--:--");

    ESP_LOGI(TAG, "font_puhui_16_4: %p", &font_puhui_16_4);
    gfx_label_set_font(obj_label_time, (gfx_font_t)&font_puhui_16_4);
    gfx_label_set_color(obj_label_time, GFX_COLOR_HEX(0xFFFFFF));
    gfx_label_set_text_align(obj_label_time, GFX_TEXT_ALIGN_CENTER);
}

static void InitializeMicAnimation(gfx_handle_t engine_handle, mmap_assets_handle_t assets_handle)
{
    obj_anim_mic = gfx_anim_create(engine_handle);
    gfx_obj_align(obj_anim_mic, GFX_ALIGN_TOP_MID, 0, 8);

    const void* anim_data = mmap_assets_get_mem(assets_handle, MMAP_EMOJI_LISTEN_32_AAF);
    size_t anim_size = mmap_assets_get_size(assets_handle, MMAP_EMOJI_LISTEN_32_AAF);
    gfx_anim_set_src(obj_anim_mic, anim_data, anim_size);
    gfx_anim_start(obj_anim_mic);
    gfx_obj_set_visible(obj_anim_mic, false);
}

static void InitializeIcon(gfx_handle_t engine_handle, mmap_assets_handle_t assets_handle)
{
    obj_img_icon = gfx_img_create(engine_handle);
    gfx_obj_align(obj_img_icon, GFX_ALIGN_TOP_LEFT, 10, 5);

    SetupImageDescriptor(assets_handle, &icon_img_dsc, MMAP_EMOJI_ICON_WIFI_FAILED_50_BIN);
    gfx_img_set_src(obj_img_icon, static_cast<void*>(&icon_img_dsc));
}

static void RegisterCallbacks(esp_lcd_panel_io_handle_t panel_io, gfx_handle_t engine_handle)
{
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = EmojiEngine::OnFlushIoReady,
    };
    esp_lcd_panel_io_register_event_callbacks(panel_io, &cbs, engine_handle);
}

// Global helper function implementation (declared in header)
void SetupImageDescriptor(mmap_assets_handle_t assets_handle,
                          gfx_image_dsc_t* img_dsc,
                          int asset_id)
{
    const void* img_data = mmap_assets_get_mem(assets_handle, asset_id);
    size_t img_size = mmap_assets_get_size(assets_handle, asset_id);

    std::memcpy(&img_dsc->header, img_data, sizeof(gfx_image_header_t));
    img_dsc->data = static_cast<const uint8_t*>(img_data) + sizeof(gfx_image_header_t);
    img_dsc->data_size = img_size - sizeof(gfx_image_header_t);
}

// EmojiEngine implementation
EmojiEngine::EmojiEngine(esp_lcd_panel_handle_t panel, esp_lcd_panel_io_handle_t panel_io)
{
    ESP_LOGI(TAG, "Create EmojiEngine, panel: %p, panel_io: %p", panel, panel_io);

    InitializeAssets(&assets_handle_);
    InitializeGraphics(panel, &engine_handle_);

    gfx_emote_lock(engine_handle_);
    gfx_emote_set_bg_color(engine_handle_, GFX_COLOR_HEX(0x000000));

    // Initialize all UI components
    InitializeEyeAnimation(engine_handle_, assets_handle_);
    InitializeLabels(engine_handle_);
    InitializeMicAnimation(engine_handle_, assets_handle_);
    InitializeIcon(engine_handle_, assets_handle_);

    // Set initial display mode
    current_icon_type = MMAP_EMOJI_ICON_WIFI_FAILED_50_BIN;
    SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);

    // Create timer for clock updates
    gfx_timer_create(engine_handle_, clock_tm_callback, 1000, obj_label_tips);

    gfx_emote_unlock(engine_handle_);

    RegisterCallbacks(panel_io, engine_handle_);
}

EmojiEngine::~EmojiEngine()
{
    if (engine_handle_) {
        gfx_emote_deinit(engine_handle_);
        engine_handle_ = nullptr;
    }

    if (assets_handle_) {
        mmap_assets_del(assets_handle_);
        assets_handle_ = nullptr;
    }
}

void EmojiEngine::setEyes(int aaf, bool repeat, int fps)
{
    if (!engine_handle_) {
        return;
    }

    const void* src_data = mmap_assets_get_mem(assets_handle_, aaf);
    size_t src_len = mmap_assets_get_size(assets_handle_, aaf);

    Lock();
    gfx_anim_set_src(obj_anim_eye, src_data, src_len);
    gfx_anim_set_segment(obj_anim_eye, 0, 0xFFFF, fps, repeat);
    gfx_anim_start(obj_anim_eye);
    Unlock();
}

void EmojiEngine::stopEyes()
{
    // Implementation if needed
}

void EmojiEngine::Lock()
{
    if (engine_handle_) {
        gfx_emote_lock(engine_handle_);
    }
}

void EmojiEngine::Unlock()
{
    if (engine_handle_) {
        gfx_emote_unlock(engine_handle_);
    }
}

void EmojiEngine::SetIcon(int asset_id)
{
    if (!engine_handle_) {
        return;
    }

    Lock();
    SetupImageDescriptor(assets_handle_, &icon_img_dsc, asset_id);
    gfx_img_set_src(obj_img_icon, static_cast<void*>(&icon_img_dsc));
    current_icon_type = asset_id;
    Unlock();
}

bool EmojiEngine::OnFlushIoReady(esp_lcd_panel_io_handle_t panel_io,
                                 esp_lcd_panel_io_event_data_t* edata,
                                 void* user_ctx)
{
    return true;
}

void EmojiEngine::OnFlush(gfx_handle_t handle, int x_start, int y_start,
                          int x_end, int y_end, const void* color_data)
{
    auto* panel = static_cast<esp_lcd_panel_handle_t>(gfx_emote_get_user_data(handle));
    if (panel) {
        esp_lcd_panel_draw_bitmap(panel, x_start, y_start, x_end, y_end, color_data);
    }
    gfx_emote_flush_ready(handle, true);
}

void EmojiEngine::OnUpdate(gfx_handle_t handle, gfx_player_event_t event)
{
    // Event handling if needed
}

// EmojiWidget implementation
EmojiWidget::EmojiWidget(esp_lcd_panel_handle_t panel, esp_lcd_panel_io_handle_t panel_io)
{
    InitializeEngine(panel, panel_io);
}

EmojiWidget::~EmojiWidget() = default;

void EmojiWidget::SetEmotion(const char* emotion)
{
    if (!engine_) {
        return;
    }

    using EmotionParam = std::tuple<int, bool, int>;
    static const std::unordered_map<std::string, EmotionParam> emotion_map = {
        {"happy",       {MMAP_EMOJI_HAPPY_ONE_32_AAF,     true,  20}},
        {"laughing",    {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"funny",       {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"loving",      {MMAP_EMOJI_HAPPY_ONE_32_AAF,     true,  20}},
        {"embarrassed", {MMAP_EMOJI_HAPPY_ONE_32_AAF,     true,  20}},
        {"confident",   {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"delicious",   {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"sad",         {MMAP_EMOJI_SAD_ONE_32_AAF,       true,  20}},
        {"crying",      {MMAP_EMOJI_SAD_ONE_32_AAF,     true,  20}},
        {"sleepy",      {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"silly",       {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"angry",       {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"surprised",   {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"shocked",     {MMAP_EMOJI_ENJOY_ONE_32_AAF,   true,  20}},
        {"thinking",    {MMAP_EMOJI_IDLE_ONE_32_AAF,  true,  20}},
        {"winking",     {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"relaxed",     {MMAP_EMOJI_IDLE_ONE_32_AAF,     true,  20}},
        {"confused",    {MMAP_EMOJI_ENJOY_ONE_32_AAF,     true,  20}},
        {"neutral",     {MMAP_EMOJI_IDLE_ONE_32_AAF,      false, 20}},
        {"idle",        {MMAP_EMOJI_IDLE_ONE_32_AAF,      false, 20}},
    };

    ESP_LOGW(TAG, "SetEmotion: %s", emotion);
    auto it = emotion_map.find(emotion);
    if (it != emotion_map.end()) {
        const auto& [aaf, repeat, fps] = it->second;
        engine_->setEyes(aaf, repeat, fps);
    }
}

void EmojiWidget::SetChatMessage(const char* role, const char* content)
{
    ESP_LOGW(TAG, "SetChatMessage(%s): %s", role, content);
    engine_->Lock();
    if (content && strlen(content) > 0) {
        gfx_label_set_text(obj_label_tips, content);
        SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
    }
    engine_->Unlock();
}

void EmojiWidget::SetStatus(const char* status)
{
    ESP_LOGW(TAG, "SetStatus: %s", status);
    if (!engine_) {
        return;
    }

    if (std::strcmp(status, "聆听中...") == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_ANIM_TOP);
        engine_->setEyes(MMAP_EMOJI_ENJOY_ONE_32_AAF, true, 20);
        engine_->SetIcon(MMAP_EMOJI_ICON_MIC_50_BIN);
    } else if (std::strcmp(status, "待命") == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_TIME);
        engine_->SetIcon(MMAP_EMOJI_ICON_BATTERY_50_BIN);
    } else if (std::strcmp(status, "说话中...") == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
        engine_->SetIcon(MMAP_EMOJI_ICON_SPEAKER_ZZZ_50_BIN);
    } else if (std::strcmp(status, "错误") == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
        engine_->SetIcon(MMAP_EMOJI_ICON_WIFI_FAILED_50_BIN);
    }

    engine_->Lock();
    if (std::strcmp(status, "连接中...") != 0) {
        gfx_label_set_text(obj_label_tips, status);
    }
    engine_->Unlock();
}

void EmojiWidget::InitializeEngine(esp_lcd_panel_handle_t panel, esp_lcd_panel_io_handle_t panel_io)
{
    engine_ = std::make_unique<EmojiEngine>(panel, panel_io);
}

bool EmojiWidget::Lock(int timeout_ms)
{
    return true;
}

void EmojiWidget::Unlock()
{
    // Implementation if needed
}

} // namespace anim
