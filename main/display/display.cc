#include <esp_log.h>
#include <esp_err.h>
#include <string>

#include "display.h"

#define TAG "Display"

Display::Display() {
    // Create a power management lock
    auto ret = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "display_update", &pm_lock_);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "Power management not supported");
    } else {
        ESP_ERROR_CHECK(ret);
    }
}

Display::~Display() {
    if (pm_lock_ != nullptr) {
        esp_pm_lock_delete(pm_lock_);
    }
}

void Display::SetStatus(const char* status) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "SetStatus: %s", status);
    last_status_update_time_ = std::chrono::system_clock::now();
}

void Display::ShowNotification(const std::string& notification, int duration_ms) {
    ShowNotification(notification.c_str(), duration_ms);
}

void Display::ShowNotification(const char* notification, int duration_ms) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "ShowNotification: %s, duration: %d", notification, duration_ms);
}

void Display::UpdateStatusBar(bool update_all) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "UpdateStatusBar: update_all=%d", update_all);
}

void Display::SetEmotion(const char* emotion) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "SetEmotion: %s", emotion);
}

void Display::SetIcon(const char* icon) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "SetIcon: %s", icon);
}

void Display::SetPreviewImage(const void* image) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "SetPreviewImage");
}

void Display::SetChatMessage(const char* role, const char* content) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "SetChatMessage: role=%s, content=%s", role ? role : "null", content ? content : "null");
}

void Display::SetTheme(const std::string& theme_name) {
    current_theme_name_ = theme_name;
    ESP_LOGI(TAG, "SetTheme: %s", theme_name.c_str());
}

void Display::SetPowerSaveMode(bool on) {
    // 基类实现，子类需要重写
    ESP_LOGI(TAG, "SetPowerSaveMode: %s", on ? "on" : "off");
    if (on) {
        SetChatMessage("system", "");
        SetEmotion("sleepy");
    } else {
        SetChatMessage("system", "");
        SetEmotion("neutral");
    }
}