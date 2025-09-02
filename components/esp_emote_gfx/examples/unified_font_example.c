/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "widget/gfx_font_parser.h"
#include "widget/gfx_label.h"

static const char *TAG = "unified_font_example";

/**
 * @brief Example showing how to use the unified font interface
 * 
 * This example demonstrates how to create and use unified font interfaces
 * that replace the previous font type checking with function pointers.
 */
void unified_font_example(void)
{
    ESP_LOGI(TAG, "Starting unified font interface example");

    // Example 1: Create unified font from LVGL font
    // Assuming you have an LVGL font structure like: extern const gfx_lvgl_font_t font_16;
    // gfx_unified_font_t *lvgl_unified_font = NULL;
    // esp_err_t ret = gfx_create_unified_font(&font_16, "lvgl_font_16", &lvgl_unified_font);
    // if (ret == ESP_OK) {
    //     ESP_LOGI(TAG, "Created unified font for LVGL font");
    //     
    //     // Use unified interface - no more type checking needed!
    //     gfx_font_glyph_dsc_t glyph_dsc;
    //     if (gfx_unified_font_get_glyph_dsc(lvgl_unified_font, &glyph_dsc, 'A', 0)) {
    //         ESP_LOGI(TAG, "LVGL font: Got glyph for 'A', width: %d", glyph_dsc.adv_w >> 8);
    //     }
    //     
    //     uint32_t width = gfx_unified_font_get_glyph_width(lvgl_unified_font, 'A');
    //     ESP_LOGI(TAG, "LVGL font: Character 'A' width: %d", width >> 8);
    //     
    //     int line_height = gfx_unified_font_get_line_height(lvgl_unified_font);
    //     ESP_LOGI(TAG, "LVGL font: Line height: %d", line_height);
    // }

    // Example 2: Create unified font from FreeType font
    // Assuming you have a FreeType face: FT_Face ft_face;
    // gfx_unified_font_t *ft_unified_font = NULL;
    // ret = gfx_create_unified_font(ft_face, "freetype_font", &ft_unified_font);
    // if (ret == ESP_OK) {
    //     ESP_LOGI(TAG, "Created unified font for FreeType font");
    //     
    //     // Use the same unified interface!
    //     gfx_font_glyph_dsc_t glyph_dsc;
    //     if (gfx_unified_font_get_glyph_dsc(ft_unified_font, &glyph_dsc, 'A', 0)) {
    //         ESP_LOGI(TAG, "FreeType font: Got glyph for 'A', width: %d", glyph_dsc.adv_w >> 8);
    //     }
    //     
    //     uint32_t width = gfx_unified_font_get_glyph_width(ft_unified_font, 'A');
    //     ESP_LOGI(TAG, "FreeType font: Character 'A' width: %d", width >> 8);
    //     
    //     int line_height = gfx_unified_font_get_line_height(ft_unified_font);
    //     ESP_LOGI(TAG, "FreeType font: Line height: %d", line_height);
    // }

    // Example 3: Using with label objects
    // The label system now automatically creates unified font interfaces
    // when you set a font using gfx_label_set_font()
    
    ESP_LOGI(TAG, "Unified font interface example completed");
}

/**
 * @brief Example showing the benefits of unified font interface
 */
void unified_font_benefits_example(void)
{
    ESP_LOGI(TAG, "Demonstrating unified font interface benefits");

    // Before (with type checking):
    // gfx_font_type_t font_type = gfx_detect_font_type(font_ptr);
    // if (font_type == GFX_FONT_TYPE_LVGL_C) {
    //     gfx_lvgl_font_t *lvgl_font = (gfx_lvgl_font_t *)font_ptr;
    //     gfx_font_glyph_dsc_t glyph_dsc;
    //     if (gfx_lvgl_font_get_glyph_dsc(lvgl_font, unicode, &glyph_dsc)) {
    //         // Process LVGL glyph
    //     }
    // } else {
    //     FT_Face ft_face = (FT_Face)font_ptr;
    //     gfx_font_glyph_dsc_t glyph_dsc;
    //     if (gfx_freetype_font_get_glyph_dsc(ft_face, font_size, unicode, &glyph_dsc)) {
    //         // Process FreeType glyph
    //     }
    // }

    // After (with unified interface):
    // gfx_unified_font_t *unified_font = NULL;
    // gfx_create_unified_font(font_ptr, "font", &unified_font);
    // 
    // gfx_font_glyph_dsc_t glyph_dsc;
    // if (gfx_unified_font_get_glyph_dsc(unified_font, &glyph_dsc, unicode, 0)) {
    //     // Process glyph - same code for both font types!
    // }

    ESP_LOGI(TAG, "Benefits demonstrated:");
    ESP_LOGI(TAG, "1. No more font type checking in rendering code");
    ESP_LOGI(TAG, "2. Same interface for all font types");
    ESP_LOGI(TAG, "3. Easier to add new font types");
    ESP_LOGI(TAG, "4. Cleaner, more maintainable code");
} 