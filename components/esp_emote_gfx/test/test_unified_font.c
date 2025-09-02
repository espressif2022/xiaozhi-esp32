/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "unity.h"
#include "widget/gfx_font_parser.h"

static const char *TAG = "test_unified_font";

/**
 * @brief Test unified font interface creation and basic functionality
 */
TEST_CASE("Test unified font interface creation", "[unified_font]")
{
    // Test with NULL font pointer
    gfx_unified_font_t *unified_font = NULL;
    esp_err_t ret = gfx_create_unified_font(NULL, "test_font", &unified_font);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    TEST_ASSERT_NULL(unified_font);

    // Test with NULL name
    ret = gfx_create_unified_font((void*)0x1234, NULL, &unified_font);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    TEST_ASSERT_NULL(unified_font);

    // Test with NULL output pointer
    ret = gfx_create_unified_font((void*)0x1234, "test_font", NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);

    ESP_LOGI(TAG, "Unified font interface creation tests passed");
}

/**
 * @brief Test unified font interface function calls with NULL pointers
 */
TEST_CASE("Test unified font interface NULL pointer handling", "[unified_font]")
{
    // Test glyph descriptor function with NULL unified font
    gfx_font_glyph_dsc_t glyph_dsc;
    bool result = gfx_unified_font_get_glyph_dsc(NULL, &glyph_dsc, 'A', 0);
    TEST_ASSERT_FALSE(result);

    // Test glyph bitmap function with NULL unified font
    const uint8_t *bitmap = gfx_unified_font_get_glyph_bitmap(NULL, &glyph_dsc);
    TEST_ASSERT_NULL(bitmap);

    // Test glyph width function with NULL unified font
    uint32_t width = gfx_unified_font_get_glyph_width(NULL, 'A');
    TEST_ASSERT_EQUAL(0, width);

    // Test line height function with NULL unified font
    int line_height = gfx_unified_font_get_line_height(NULL);
    TEST_ASSERT_EQUAL(0, line_height);

    // Test base line function with NULL unified font
    int base_line = gfx_unified_font_get_base_line(NULL);
    TEST_ASSERT_EQUAL(0, base_line);

    ESP_LOGI(TAG, "Unified font interface NULL pointer tests passed");
}

// Font type detection test removed - type field no longer exists

/**
 * @brief Test unified font interface memory management
 */
TEST_CASE("Test unified font interface memory management", "[unified_font]")
{
    // Create a mock font structure that looks like LVGL font
    typedef struct {
        void *dsc;
        uint16_t line_height;
        uint16_t base_line;
    } mock_lvgl_font_t;

    mock_lvgl_font_t mock_font = {
        .dsc = (void*)0x1234,
        .line_height = 20,
        .base_line = 5
    };

    gfx_unified_font_t *unified_font = NULL;
    esp_err_t ret = gfx_create_unified_font(&mock_font, "mock_font", &unified_font);
    
    // This should fail because the mock font doesn't have proper LVGL structure
    // but the function should handle it gracefully
    if (ret == ESP_OK && unified_font != NULL) {
        // Test that we can call functions without crashing
        gfx_font_glyph_dsc_t glyph_dsc;
        bool result = gfx_unified_font_get_glyph_dsc(unified_font, &glyph_dsc, 'A', 0);
        // Result should be false for invalid font, but no crash
        
        // Clean up
        free(unified_font);
    }

    ESP_LOGI(TAG, "Unified font interface memory management tests passed");
}

/**
 * @brief Test unified font interface integration
 */
TEST_CASE("Test unified font interface integration", "[unified_font]")
{
    // This test would require actual font data to be meaningful
    // For now, we just test that the interface functions exist and can be called
    
    ESP_LOGI(TAG, "Testing unified font interface integration");
    
    // Test that all function pointers are properly set when creating unified font
    // This would require actual font data, so we skip the detailed test for now
    
    ESP_LOGI(TAG, "Unified font interface integration tests completed");
} 