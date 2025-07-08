import os
from PIL import Image

def merge_images(input_dir, output_path):
    # 配置参数
    img_size = 32  # 单张图片尺寸32×32
    target_width = 160  # 目标宽度
    target_height = 224  # 目标高度
    per_row = target_width // img_size  # 每行放5张
    total_images = 35  # 总图片数
    
    # 1. 获取输入图片列表（按文件名排序，确保顺序正确）
    # 假设图片文件名按顺序命名（如1.png, 2.png...35.png）
    image_files = [f for f in os.listdir(input_dir) 
                  if f.lower().endswith(('.png', '.jpg', '.jpeg', '.gif', '.bmp'))]
    # 按文件名数字排序（关键：确保顺序正确）
    image_files.sort(key=lambda x: int(''.join(filter(str.isdigit, x))))
    
    # 检查图片数量是否正确
    if len(image_files) != total_images:
        raise ValueError(f"{input_dir}需要35张图片，实际找到{len(image_files)}张")
    
    # 2. 创建目标画布（RGBA支持透明背景）
    merged_img = Image.new('RGBA', (target_width, target_height), (0, 0, 0, 0))
    
    # 3. 按顺序粘贴图片
    for i, img_file in enumerate(image_files):
        # 计算当前图片位置
        row = i // per_row  # 行索引（0-6）
        col = i % per_row   # 列索引（0-4）
        x = col * img_size  # X坐标
        y = row * img_size  # Y坐标
        
        # 打开图片并粘贴
        img_path = os.path.join(input_dir, img_file)
        with Image.open(img_path) as img:
            # 确保图片是32×32（若不是则缩放，可选）
            if img.size != (img_size, img_size):
                img = img.resize((img_size, img_size), Image.LANCZOS)
            merged_img.paste(img, (x, y))
    
    # 4. 保存合并后的图片
    merged_img.save(output_path)
    print(f"图片合并完成，已保存至：{output_path}")

if __name__ == "__main__":
    # 配置输入文件夹和输出路径
    input_directory = "D:/program/wfc/ModernWFC/ModernWFC/assets/新建文件夹/images"  # 存放35张32×32图片的文件夹
    output_file = "D:/program/wfc/ModernWFC/ModernWFC/assets/新建文件夹/merge.png"  # 输出的160×224图片
    
    # 确保输入文件夹存在
    if not os.path.exists(input_directory):
        os.makedirs(input_directory)
    
    # 执行合并
    merge_images(input_directory, output_file)