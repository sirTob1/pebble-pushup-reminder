from PIL import Image
import os

def create_icons():
    try:
        img = Image.open("assets/store/raw_icon.png")
        
        # Resize to 80x80 for store
        img_80 = img.resize((80, 80), Image.Resampling.LANCZOS)
        img_80.save("assets/store/icon_80x80.png")
        
        # Resize to 144x144 for store
        img_144 = img.resize((144, 144), Image.Resampling.LANCZOS)
        img_144.save("assets/store/icon_144x144.png")
        
        # Generate 25x25 watch launcher menu icon
        os.makedirs("resources/images", exist_ok=True)
        rgb = img.convert("RGB")
        menu_icon = Image.new("RGBA", (25, 25), (0, 0, 0, 0))
        crop_x, crop_y, crop_w, crop_h = 198.0, 294.0, 628.0, 435.0
        dest_x0, dest_w, dest_y0, dest_h = 1, 23, 5, 16
        for dy in range(dest_h):
            for dx in range(dest_w):
                sx0 = int(crop_x + (dx / dest_w) * crop_w)
                sx1 = int(crop_x + ((dx + 1) / dest_w) * crop_w)
                sy0 = int(crop_y + (dy / dest_h) * crop_h)
                sy1 = int(crop_y + ((dy + 1) / dest_h) * crop_h)
                white_count = 0
                total_count = 0
                for sy in range(sy0, sy1, 2):
                    for sx in range(sx0, sx1, 2):
                        r, g, b = rgb.getpixel((sx, sy))
                        if g > 140 and b > 140:
                            white_count += 1
                        total_count += 1
                if total_count > 0 and (white_count / total_count) > 0.32:
                    menu_icon.putpixel((dest_x0 + dx, dest_y0 + dy), (0, 0, 0, 255))
        menu_icon.putpixel((15, 8), (0, 0, 0, 255))
        menu_icon.save("resources/images/menu_icon.png")
        
        print("Successfully generated icon_80x80.png, icon_144x144.png, and resources/images/menu_icon.png")
    except Exception as e:
        print("Error creating icons:", e)

create_icons()
