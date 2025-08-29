#ifndef EMOTE_DISPLAY_H
#define EMOTE_DISPLAY_H

#include "display.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include "esp_mmap_assets.h"
#include "gfx.h"

namespace anim {

enum class UIDisplayMode : uint8_t {
    SHOW_ANIM_TOP = 1,  // Show obj_anim_mic
    SHOW_TIME = 2,      // Show obj_label_time
    SHOW_TIPS = 3       // Show obj_label_tips
};

class EmoteDisplay : public Display {
protected:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    
    // GFX 相关元素，从 EmoteEngine 移到这里
    gfx_handle_t engine_handle_ = nullptr;
    mmap_assets_handle_t assets_handle_ = nullptr;
    
    // GFX UI 元素
    gfx_obj_t* obj_label_tips_ = nullptr;
    gfx_obj_t* obj_label_time_ = nullptr;
    gfx_obj_t* obj_anim_eye_ = nullptr;
    gfx_obj_t* obj_anim_mic_ = nullptr;
    gfx_obj_t* obj_img_icon_ = nullptr;
    
    // 图标相关
    gfx_image_dsc_t icon_img_dsc_;
    int current_icon_type_;
    
    // 字体配置
    DisplayFonts fonts_;
    
    // 主题相关
    std::string current_theme_name_;

    void SetupUI(int width, int height);
    void SetUIDisplayMode(UIDisplayMode mode);
    void SetEyes(int aaf, bool repeat, int fps);
    void SetIcon(int asset_id);
    void SetImageDesc(gfx_image_dsc_t* img_dsc, int asset_id);
    static void clock_tm_callback(void* user_data);
    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

public:
    // Callback functions (public to be accessible from static helper functions)
    static bool OnFlushIoReady(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
    static void OnFlush(gfx_handle_t handle, int x_start, int y_start, int x_end, int y_end, const void *color_data);

protected:
    // 添加protected构造函数
    EmoteDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, DisplayFonts fonts, mmap_assets_handle_t assets_handle);
    
    
public:
    virtual ~EmoteDisplay();

    // 实现 Display 接口的所有方法
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetStatus(const char* status) override;
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void SetIcon(const char* icon) override;
    virtual void SetPreviewImage(const void* image) override;
    virtual void SetTheme(const std::string& theme_name) override;
    virtual void ShowNotification(const char* notification, int duration_ms = 3000) override;
    virtual void ShowNotification(const std::string& notification, int duration_ms = 3000) override;
    virtual void UpdateStatusBar(bool update_all = false) override;
    virtual void SetPowerSaveMode(bool on) override;
};

class SPIEmoteDisplay : public EmoteDisplay {
    public:
        SPIEmoteDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                      int width, int height,
                      DisplayFonts fonts, mmap_assets_handle_t assets_handle);
};

} // namespace anim

#endif // EMOTE_DISPLAY_H