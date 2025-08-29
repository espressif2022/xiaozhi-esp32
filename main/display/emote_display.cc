
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

#include "config.h"
#include "assets/lang_config.h"
#include "display/lcd_display.h"
#include "emote_display.h"

namespace anim {

static const char* TAG = "emoji";


enum MMAP_EMOJI_NORMAL_LISTS {
    MMAP_EMOJI_NORMAL_ANGRY_ONE_AAF = 0,        /*!< angry_one.aaf */
    MMAP_EMOJI_NORMAL_DIZZY_ONE_AAF = 1,        /*!< dizzy_one.aaf */
    MMAP_EMOJI_NORMAL_ENJOY_ONE_AAF = 2,        /*!< enjoy_one.aaf */
    MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF = 3,        /*!< happy_one.aaf */
    MMAP_EMOJI_NORMAL_IDLE_ONE_AAF = 4,        /*!< idle_one.aaf */
    MMAP_EMOJI_NORMAL_LISTEN_AAF = 5,        /*!< listen.aaf */
    MMAP_EMOJI_NORMAL_SAD_ONE_AAF = 6,        /*!< sad_one.aaf */
    MMAP_EMOJI_NORMAL_SHOCKED_ONE_AAF = 7,        /*!< shocked_one.aaf */
    MMAP_EMOJI_NORMAL_THINKING_ONE_AAF = 8,        /*!< thinking_one.aaf */
    MMAP_EMOJI_NORMAL_ICON_BATTERY_BIN = 9,        /*!< icon_Battery.bin */
    MMAP_EMOJI_NORMAL_ICON_WIFI_FAILED_BIN = 10,        /*!< icon_WiFi_failed.bin */
    MMAP_EMOJI_NORMAL_ICON_MIC_BIN = 11,        /*!< icon_mic.bin */
    MMAP_EMOJI_NORMAL_ICON_SPEAKER_ZZZ_BIN = 12,        /*!< icon_speaker_zzz.bin */
    MMAP_EMOJI_NORMAL_ICON_WIFI_BIN = 13,        /*!< icon_wifi.bin */
};

bool EmoteDisplay::OnFlushIoReady(esp_lcd_panel_io_handle_t panel_io,
                                 esp_lcd_panel_io_event_data_t* edata,
                                 void* user_ctx)
{
    return true;
}

void EmoteDisplay::OnFlush(gfx_handle_t handle, int x_start, int y_start,
                          int x_end, int y_end, const void* color_data)
{
    auto* panel = static_cast<esp_lcd_panel_handle_t>(gfx_emote_get_user_data(handle));
    if (panel) {
        esp_lcd_panel_draw_bitmap(panel, x_start, y_start, x_end, y_end, color_data);
    }
    gfx_emote_flush_ready(handle, true);
}

void EmoteDisplay::SetUIDisplayMode(UIDisplayMode mode)
{
    gfx_obj_set_visible(obj_anim_mic_, false);
    gfx_obj_set_visible(obj_label_time_, false);
    gfx_obj_set_visible(obj_label_tips_, false);

    // Show the selected control
    switch (mode) {
    case UIDisplayMode::SHOW_ANIM_TOP:
        gfx_obj_set_visible(obj_anim_mic_, true);
        break;
    case UIDisplayMode::SHOW_TIME:
        gfx_obj_set_visible(obj_label_time_, true);
        break;
    case UIDisplayMode::SHOW_TIPS:
        gfx_obj_set_visible(obj_label_tips_, true);
        break;
    }
}

void EmoteDisplay::clock_tm_callback(void* user_data)
{
    EmoteDisplay* display = static_cast<EmoteDisplay*>(user_data);
    if (display->current_icon_type_ == MMAP_EMOJI_NORMAL_ICON_BATTERY_BIN) {
        time_t now;
        struct tm timeinfo;
        time(&now);

        setenv("TZ", "GMT+0", 1);
        tzset();
        localtime_r(&now, &timeinfo);

        char time_str[6];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

        gfx_label_set_text(display->obj_label_time_, time_str);
        display->SetUIDisplayMode(UIDisplayMode::SHOW_TIME);
    }
}

void EmoteDisplay::SetImageDesc(gfx_image_dsc_t* img_dsc, int asset_id)
{
    const void* img_data = mmap_assets_get_mem(assets_handle_, asset_id);
    size_t img_size = mmap_assets_get_size(assets_handle_, asset_id);

    std::memcpy(&img_dsc->header, img_data, sizeof(gfx_image_header_t));
    img_dsc->data = static_cast<const uint8_t*>(img_data) + sizeof(gfx_image_header_t);
    img_dsc->data_size = img_size - sizeof(gfx_image_header_t);
}

EmoteDisplay::EmoteDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, DisplayFonts fonts, mmap_assets_handle_t assets_handle)
    : panel_io_(panel_io), panel_(panel), assets_handle_(assets_handle), current_icon_type_(MMAP_EMOJI_NORMAL_ICON_BATTERY_BIN), fonts_(fonts)
{
    ESP_LOGI(TAG, "EmoteDisplay base constructor, width: %d, height: %d", width, height);
}

SPIEmoteDisplay::SPIEmoteDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                                 int width, int height,
                                 DisplayFonts fonts, mmap_assets_handle_t assets_handle)
    : EmoteDisplay(panel_io, panel, width, height, fonts, assets_handle)
{
    ESP_LOGI(TAG, "initializing GFX engine and UI components");

    // Initialize GFX engine
    gfx_core_config_t gfx_cfg = {
        .flush_cb = EmoteDisplay::OnFlush,
        .user_data = panel_,
        .flags = {
            .swap = true,
            .double_buffer = true,
            .buff_dma = true,
        },
        .h_res = static_cast<uint32_t>(width),
        .v_res = static_cast<uint32_t>(height),
        .fps = 30,
        .buffers = {
            .buf1 = nullptr,
            .buf2 = nullptr,
            .buf_pixels = static_cast<size_t>(width * 16),
        },
        .task = GFX_EMOTE_INIT_CONFIG()
    };
    gfx_cfg.task.task_stack_caps = MALLOC_CAP_DEFAULT;
    gfx_cfg.task.task_affinity = 0;
    gfx_cfg.task.task_priority = 5;
    gfx_cfg.task.task_stack = 6 * 1024;
    engine_handle_ = gfx_emote_init(&gfx_cfg);

    // Register callbacks
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = EmoteDisplay::OnFlushIoReady,
    };
    esp_lcd_panel_io_register_event_callbacks(panel_io_, &cbs, engine_handle_);
    
    SetupUI(width, height);
}

void EmoteDisplay::SetupUI(int width, int height)
{
    ESP_LOGI(TAG, "Setting up UI components");

    DisplayLockGuard lock(this);

    gfx_emote_set_bg_color(engine_handle_, GFX_COLOR_HEX(0x000000));
    
    // Initialize eye animation
    obj_anim_eye_ = gfx_anim_create(engine_handle_);
    const void* eye_anim_data = mmap_assets_get_mem(assets_handle_, MMAP_EMOJI_NORMAL_IDLE_ONE_AAF);
    size_t eye_anim_size = mmap_assets_get_size(assets_handle_, MMAP_EMOJI_NORMAL_IDLE_ONE_AAF);
    gfx_anim_set_src(obj_anim_eye_, eye_anim_data, eye_anim_size);
    // gfx_obj_align(obj_anim_eye_, GFX_ALIGN_LEFT_MID, 10, -20);
    gfx_obj_align(obj_anim_eye_, GFX_ALIGN_LEFT_MID, 0, 20);
    gfx_anim_set_auto_mirror(obj_anim_eye_, true);
    gfx_anim_set_segment(obj_anim_eye_, 0, 0xFFFF, 20, false);
    gfx_anim_start(obj_anim_eye_);

    // Initialize tips label
    obj_label_tips_ = gfx_label_create(engine_handle_);
    gfx_obj_align(obj_label_tips_, GFX_ALIGN_TOP_MID, 0, 40);
    gfx_obj_set_size(obj_label_tips_, 160, 40);
    gfx_label_set_text(obj_label_tips_, Lang::Strings::INITIALIZING);
    gfx_label_set_color(obj_label_tips_, GFX_COLOR_HEX(0xFFFFFF));
    gfx_label_set_text_align(obj_label_tips_, GFX_TEXT_ALIGN_CENTER);
    gfx_label_set_long_mode(obj_label_tips_, GFX_LABEL_LONG_SCROLL);
    gfx_label_set_scroll_speed(obj_label_tips_, 20);
    gfx_label_set_scroll_loop(obj_label_tips_, true);
    gfx_label_set_font(obj_label_tips_, (gfx_font_t)fonts_.text_font);

    // Initialize time label
    obj_label_time_ = gfx_label_create(engine_handle_);
    gfx_obj_align(obj_label_time_, GFX_ALIGN_TOP_MID, 0, 40);
    gfx_obj_set_size(obj_label_time_, 160, 50);
    gfx_label_set_text(obj_label_time_, "--:--");
    gfx_label_set_color(obj_label_time_, GFX_COLOR_HEX(0xFFFFFF));
    gfx_label_set_text_align(obj_label_time_, GFX_TEXT_ALIGN_CENTER);
    gfx_label_set_font(obj_label_time_, (gfx_font_t)fonts_.text_font);

    // Initialize mic animation
    obj_anim_mic_ = gfx_anim_create(engine_handle_);
    gfx_obj_align(obj_anim_mic_, GFX_ALIGN_TOP_MID, 0, 25);
    const void* mic_anim_data = mmap_assets_get_mem(assets_handle_, MMAP_EMOJI_NORMAL_LISTEN_AAF);
    size_t mic_anim_size = mmap_assets_get_size(assets_handle_, MMAP_EMOJI_NORMAL_LISTEN_AAF);
    gfx_anim_set_src(obj_anim_mic_, mic_anim_data, mic_anim_size);
    gfx_anim_start(obj_anim_mic_);
    gfx_obj_set_visible(obj_anim_mic_, false);

    // Initialize icon
    obj_img_icon_ = gfx_img_create(engine_handle_);
    gfx_obj_align(obj_img_icon_, GFX_ALIGN_TOP_MID, -100, 38);
    SetImageDesc(&icon_img_dsc_, MMAP_EMOJI_NORMAL_ICON_WIFI_FAILED_BIN);
    gfx_img_set_src(obj_img_icon_, static_cast<void*>(&icon_img_dsc_));
    
    // Set initial state
    current_icon_type_ = MMAP_EMOJI_NORMAL_ICON_WIFI_FAILED_BIN;
    SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
    
    // Create timer for clock updates
    gfx_timer_create(engine_handle_, clock_tm_callback, 1000, this);
}

EmoteDisplay::~EmoteDisplay()
{
    if (engine_handle_) {
        gfx_emote_deinit(engine_handle_);
        engine_handle_ = nullptr;
    }

    // Note: assets_handle_ is now managed by the board layer, not deleted here
}

void EmoteDisplay::SetEmotion(const char* emotion)
{
    if (!engine_handle_) {
        return;
    }

    DisplayLockGuard lock(this);

    ESP_LOGW(TAG, "SetEmotion: %s", emotion);

    using EmotionParam = std::tuple<int, bool, int>;
    static const std::unordered_map<std::string, EmotionParam> emotion_map = {
        {"happy",       {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"laughing",    {MMAP_EMOJI_NORMAL_ENJOY_ONE_AAF,     true,  20}},
        {"funny",       {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"loving",      {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"embarrassed", {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"confident",   {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"delicious",   {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"sad",         {MMAP_EMOJI_NORMAL_SAD_ONE_AAF,       true,  20}},
        {"crying",      {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"sleepy",      {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"silly",       {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"angry",       {MMAP_EMOJI_NORMAL_ANGRY_ONE_AAF,     true,  20}},
        {"surprised",   {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"shocked",     {MMAP_EMOJI_NORMAL_SHOCKED_ONE_AAF,   true,  20}},
        {"thinking",    {MMAP_EMOJI_NORMAL_THINKING_ONE_AAF,  true,  20}},
        {"winking",     {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"relaxed",     {MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF,     true,  20}},
        {"confused",    {MMAP_EMOJI_NORMAL_DIZZY_ONE_AAF,     true,  20}},
        {"neutral",     {MMAP_EMOJI_NORMAL_IDLE_ONE_AAF,      false, 20}},
        {"idle",        {MMAP_EMOJI_NORMAL_IDLE_ONE_AAF,      false, 20}},
    };

    auto it = emotion_map.find(emotion);
    if (it != emotion_map.end()) {
        int aaf = std::get<0>(it->second);
        bool repeat = std::get<1>(it->second);
        int fps = std::get<2>(it->second);
        SetEyes(aaf, repeat, fps);
    }
}

void EmoteDisplay::SetEyes(int aaf, bool repeat, int fps)
{
    if (!engine_handle_) {
        return;
    }

    const void* src_data = mmap_assets_get_mem(assets_handle_, aaf);
    size_t src_len = mmap_assets_get_size(assets_handle_, aaf);

    gfx_anim_set_src(obj_anim_eye_, src_data, src_len);
    gfx_anim_set_segment(obj_anim_eye_, 0, 0xFFFF, fps, repeat);
    gfx_anim_start(obj_anim_eye_);
}

void EmoteDisplay::SetIcon(int asset_id)
{
    if (!engine_handle_) {
        return;
    }

    SetImageDesc(&icon_img_dsc_, asset_id);
    gfx_img_set_src(obj_img_icon_, static_cast<void*>(&icon_img_dsc_));
    current_icon_type_ = asset_id;
}

void EmoteDisplay::SetChatMessage(const char* role, const char* content)
{
    DisplayLockGuard lock(this);

    ESP_LOGW(TAG, "SetChatMessage: %s", content);

    if (content && strlen(content) > 0) {
        std::string processed_content = content;
        
        size_t pos = 0;
        while ((pos = processed_content.find('\n', pos)) != std::string::npos) {
            processed_content.replace(pos, 1, " ");
            pos += 1; // 移动到下一个位置
        }
        
        gfx_label_set_text(obj_label_tips_, processed_content.c_str());
        SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
    }
}

void EmoteDisplay::SetStatus(const char* status)
{
    if (!engine_handle_) {
        return;
    }
    DisplayLockGuard lock(this);

    ESP_LOGW(TAG, "SetStatus: %s", status);

    if (std::strcmp(status, Lang::Strings::LISTENING) == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_ANIM_TOP);
        SetEyes(MMAP_EMOJI_NORMAL_HAPPY_ONE_AAF, true, 20);
        SetIcon(MMAP_EMOJI_NORMAL_ICON_MIC_BIN);
    } else if (std::strcmp(status, Lang::Strings::STANDBY) == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_TIME);
        SetIcon(MMAP_EMOJI_NORMAL_ICON_BATTERY_BIN);
    } else if (std::strcmp(status, Lang::Strings::SPEAKING) == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
        SetIcon(MMAP_EMOJI_NORMAL_ICON_SPEAKER_ZZZ_BIN);
    } else if (std::strcmp(status, Lang::Strings::ERROR) == 0) {
        SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
        SetIcon(MMAP_EMOJI_NORMAL_ICON_WIFI_FAILED_BIN);
    }

    if (std::strcmp(status, Lang::Strings::CONNECTING) != 0) {
        gfx_label_set_text(obj_label_tips_, status);
    }
}

bool EmoteDisplay::Lock(int timeout_ms)
{
    if (engine_handle_) {
        gfx_emote_lock(engine_handle_);
        return true;
    }
    return false;
}

void EmoteDisplay::Unlock()
{
    if (engine_handle_) {
        gfx_emote_unlock(engine_handle_);
    }
}

void EmoteDisplay::SetIcon(const char* icon)
{
    if (!icon || !engine_handle_) {
        return;
    }
    
    // 将图标名称映射到资源ID
    if (std::strcmp(icon, "wifi") == 0) {
        SetIcon(MMAP_EMOJI_NORMAL_ICON_WIFI_BIN);
    } else if (std::strcmp(icon, "battery") == 0) {
        SetIcon(MMAP_EMOJI_NORMAL_ICON_BATTERY_BIN);
    } else if (std::strcmp(icon, "mic") == 0) {
        SetIcon(MMAP_EMOJI_NORMAL_ICON_MIC_BIN);
    } else if (std::strcmp(icon, "speaker") == 0) {
        SetIcon(MMAP_EMOJI_NORMAL_ICON_SPEAKER_ZZZ_BIN);
    } else if (std::strcmp(icon, "error") == 0) {
        SetIcon(MMAP_EMOJI_NORMAL_ICON_WIFI_FAILED_BIN);
    }
}

void EmoteDisplay::SetPreviewImage(const void* image)
{
    // EmoteDisplay 不支持预览图片，使用默认图标
    if (image) {
        ESP_LOGI(TAG, "EmoteDisplay: Preview image not supported, using default icon");
        SetIcon(MMAP_EMOJI_NORMAL_ICON_BATTERY_BIN);
    }
}

void EmoteDisplay::SetTheme(const std::string& theme_name)
{
    current_theme_name_ = theme_name;
    ESP_LOGI(TAG, "EmoteDisplay: Theme set to %s", theme_name.c_str());
    
    // 根据主题设置背景色
    if (engine_handle_) {
        DisplayLockGuard lock(this);
        if (theme_name == "dark") {
            gfx_emote_set_bg_color(engine_handle_, GFX_COLOR_HEX(0x000000));
        } else if (theme_name == "light") {
            gfx_emote_set_bg_color(engine_handle_, GFX_COLOR_HEX(0xFFFFFF));
        }
    }
}

void EmoteDisplay::ShowNotification(const char* notification, int duration_ms)
{
    if (!notification || !engine_handle_) {
        return;
    }
    
    ESP_LOGW(TAG, "EmoteDisplay: Show notification: %s", notification);
    
    DisplayLockGuard lock(this);
    gfx_label_set_text(obj_label_tips_, notification);
    SetUIDisplayMode(UIDisplayMode::SHOW_TIPS);
}

void EmoteDisplay::ShowNotification(const std::string& notification, int duration_ms)
{
    ShowNotification(notification.c_str(), duration_ms);
}

void EmoteDisplay::UpdateStatusBar(bool update_all)
{
    // EmoteDisplay 使用自己的状态显示方式，不需要更新状态栏
}

void EmoteDisplay::SetPowerSaveMode(bool on)
{
    if (engine_handle_) {
        DisplayLockGuard lock(this);

        ESP_LOGW(TAG, "SetPowerSaveMode: %d", on);
        if (on) {
            // 进入省电模式：降低亮度或停止动画
            gfx_anim_stop(obj_anim_eye_);
        } else {
            // 退出省电模式：恢复动画
            gfx_anim_start(obj_anim_eye_);
        }
    }
}

} // namespace anim
