import os
from PIL import Image

def expand_to_32x32_left(image_path, output_path):
    # 打开原图并转为RGBA模式（支持透明）
    with Image.open(image_path) as img:
        img = img.convert("RGBA")
        orig_width, orig_height = img.size
        
        # 创建32×32的透明画布
        new_img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        
        # 计算原图位置：左侧对齐（x=0），垂直方向居中
        # 垂直居中 = (新图高度 - 原图高度) ÷ 2
        y_position = (32 - orig_height) // 2
        
        # 粘贴原图到左侧居中位置
        new_img.paste(img, (0, y_position))
        
        # 保存结果
        new_img.save(output_path)
        print(f"已生成32×32图片，原图居左，保存为：{output_path}")

if __name__ == "__main__":
    # 获取脚本所在文件夹路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 输入图片名（确保与脚本同文件夹）
    input_img = os.path.join(script_dir, "apple.png")  # 替换为你的图片名
    output_img = os.path.join(script_dir, "expanded_left_32x32.png")
    
    # 执行拓展操作
    expand_to_32x32_left(input_img, output_img)