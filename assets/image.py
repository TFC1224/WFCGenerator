import os
from PIL import Image

# 1. 获取当前脚本所在文件夹的绝对路径
script_folder = os.path.dirname(os.path.abspath(__file__))
# 2. 拼接图片的完整路径（确保图片和脚本在同一文件夹）
image_path = os.path.join(script_folder, "apple.png")

# 3. 打开图片（此时会在脚本所在文件夹中查找）
img = Image.open(image_path)

# 4. 后续操作不变
new_img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
x = (32 - 10) // 2
y = (32 - 12) // 2
new_img.paste(img, (x, y))

# 保存时也用脚本所在文件夹路径（可选，确保输出也在同目录）
output_path = os.path.join(script_folder, "new_apple.png")
new_img.save(output_path)