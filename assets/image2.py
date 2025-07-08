import os
from PIL import Image

def resize_and_pad(image_path, output_path, target_size=(32, 32)):
    """将图片等比例缩放并居中填充到目标尺寸，空白处使用透明背景"""
    # 打开原图并转换为RGBA模式(支持透明通道)
    with Image.open(image_path) as img:
        img = img.convert("RGBA")
        
        # 计算原图宽高比
        width, height = img.size
        ratio = min(target_size[0] / width, target_size[1] / height)
        
        # 计算等比例缩放后的尺寸
        new_size = (int(width * ratio), int(height * ratio))
        
        # 缩放图片(使用高质量抗锯齿)
        resized_img = img.resize(new_size, Image.LANCZOS)
        
        # 创建透明背景的目标画布
        result = Image.new("RGBA", target_size, (0, 0, 0, 0))
        
        # 计算居中位置并粘贴缩放后的图片
        position = ((target_size[0] - new_size[0]) // 2, 
                    (target_size[1] - new_size[1]) // 2)
        result.paste(resized_img, position)
        
        # 保存结果
        result.save(output_path)
        print(f"已将图片从 {width}×{height} 缩放至 {new_size}，并填充到 {target_size}")

if __name__ == "__main__":
    # 获取当前脚本所在文件夹路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 设置输入输出文件路径(确保图片在同一文件夹)
    input_image = os.path.join(script_dir, "apple.png")
    output_image = os.path.join(script_dir, "apple_scaled.png")
    
    # 执行缩放和填充
    resize_and_pad(input_image, output_image)