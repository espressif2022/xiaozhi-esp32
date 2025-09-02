import os

# 定义要生成的字体配置
# 字符集选项: "all" = 所有字符(含完整GB2312), "basic" = 中英文基础字符
font_configs = [
    (14, 1, "all"),   # 14号字体，1 bpp，所有字符
    (16, 4, "all"),    # 16号字体，4 bpp，所有字符
    (20, 4, "all"),    # 20号字体，4 bpp，所有字符
    (30, 4, "all"),  # 30号字体，4 bpp，所有字符
    (20, 4, "basic"),  # 20号字体，4 bpp，英文数字标准字符
    (40, 4, "basic"),  # 30号字体，4 bpp，英文数字标准字符
]

def main():
    # 遍历所有字体配置
    for size, bpp, charset in font_configs:
        charset_desc = "所有字符" if charset == "all" else "中英文基础字符"
        print(f"\n正在生成 {size}px 字体，{bpp} bpp，{charset_desc}...")
        
        # 构建命令并执行
        cmd = f"python font.py lvgl --font-size {size} --bpp {bpp} --charset {charset}"
        ret = os.system(cmd)
        
        if ret != 0:
            print(f"生成 {size}px 字体失败")
        else:
            print(f"生成 {size}px 字体成功")

if __name__ == "__main__":
    main() 