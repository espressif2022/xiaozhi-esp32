#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// PuHui 字体声明
// 所有字符集 (包含完整 GB2312)
LV_FONT_DECLARE(font_puhui_14_1);   // 14px, 1bpp, 所有字符
LV_FONT_DECLARE(font_puhui_16_4);   // 16px, 4bpp, 所有字符
LV_FONT_DECLARE(font_puhui_20_4);   // 20px, 4bpp, 所有字符
LV_FONT_DECLARE(font_puhui_30_4);   // 30px, 4bpp, 所有字符

// 基础字符集 (英文数字标准字符)
LV_FONT_DECLARE(font_puhui_25_4_basic);  // 25px, 4bpp, 基础字符
LV_FONT_DECLARE(font_puhui_40_4_basic);  // 40px, 4bpp, 基础字符

#ifdef __cplusplus
}
#endif
