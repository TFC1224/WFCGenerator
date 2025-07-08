import os
from PIL import Image

def expand_to_32x32_right(image_path, output_path):
    # 打开原图并转为RGBA模式（支持透明通道）
    with Image.open(image_path) as img:
        img = img.convert("RGBA")
        orig_width, orig_height = img.size
        
        # 检查原图宽度是否超过32（若超过则按比例缩小宽度至32，避免超出）
        if orig_width > 32:
            scale_ratio = 32 / orig_width
            new_size = (32, int(orig_height * scale_ratio))
            img = img.resize(new_size, Image.LANCZOS)  # 高质量缩放
            orig_width, orig_height = new_size
        
        # 创建32×32透明画布
        new_img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        
        # 计算原图位置：右侧对齐（x坐标=32-原图宽度），垂直居中
        x_position = 32 - orig_width  # 右侧紧贴，左侧留白
        y_position = (32 - orig_height) // 2  # 垂直居中
        
        # 粘贴原图到右侧居中位置
        new_img.paste(img, (x_position, y_position))
        
        # 保存结果
        new_img.save(output_path)
        print(f"已生成32×32图片，原图居右，保存为：{output_path}")

if __name__ == "__main__":
    # 获取脚本所在文件夹路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 输入输出路径（替换为你的图片名）
    input_img = os.path.join(script_dir, "apple.png")
    output_img = os.path.join(script_dir, "expanded_right_32x32.png")
    
    # 执行拓展操作
    expand_to_32x32_right(input_img, output_img)