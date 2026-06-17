from PIL import Image, ImageDraw, ImageFont
import os
import glob

# Create assets directory
os.makedirs("assets/store", exist_ok=True)

# Define platforms
platforms = {
    "basalt": {"size": (144, 168), "bg": "white"},
    "chalk": {"size": (180, 180), "bg": "white", "is_round": True},
    "emery": {"size": (200, 228), "bg": "white"}
}

def get_font(size, is_bold=False):
    try:
        if is_bold:
            return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size)
        else:
            return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size)
    except:
        return ImageFont.load_default()

def draw_header(draw, w, header_h, text, bg_color=(255, 0, 0), text_color="white", is_round=False):
    draw.rectangle([0, 0, w, header_h], fill=bg_color)
    font = get_font(20 if not is_round else 24, True)
    bbox = draw.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    draw.text(((w - tw)/2, (header_h - th)/2 - 2), text, fill=text_color, font=font)

def draw_screen_1(draw, size, is_round):
    # Main Progress Screen
    w, h = size
    header_h = 36 if not is_round else 46
    draw_header(draw, w, header_h, "PUSHUPS", is_round=is_round)
    
    font_numbers = get_font(36 if not is_round else 42, True)
    prog_text = "15 / 30"
    bbox2 = draw.textbbox((0, 0), prog_text, font=font_numbers)
    tw2 = bbox2[2] - bbox2[0]
    draw.text(((w - tw2)/2, header_h + 20), prog_text, fill="black", font=font_numbers)
    
    font_sub = get_font(16)
    sub_text = "Tagesziel"
    bbox3 = draw.textbbox((0, 0), sub_text, font=font_sub)
    tw3 = bbox3[2] - bbox3[0]
    draw.text(((w - tw3)/2, header_h + 65 if not is_round else header_h + 80), sub_text, fill=(100,100,100), font=font_sub)

def draw_screen_2(draw, size, is_round):
    # Quick Log Screen
    w, h = size
    header_h = 36 if not is_round else 46
    draw_header(draw, w, header_h, "QUICK LOG", bg_color=(0, 0, 0), is_round=is_round)
    
    # Draw UP arrow
    draw.polygon([(w/2, header_h + 10), (w/2 - 10, header_h + 20), (w/2 + 10, header_h + 20)], fill="black")
    
    # Draw Number
    font_numbers = get_font(48 if not is_round else 56, True)
    prog_text = "5"
    bbox2 = draw.textbbox((0, 0), prog_text, font=font_numbers)
    tw2 = bbox2[2] - bbox2[0]
    draw.text(((w - tw2)/2, header_h + 25), prog_text, fill=(255,0,0), font=font_numbers)
    
    # Draw DOWN arrow
    draw.polygon([(w/2, h - 30), (w/2 - 10, h - 40), (w/2 + 10, h - 40)], fill="black")

def draw_screen_3(draw, size, is_round):
    # Reminder Screen
    w, h = size
    header_h = 36 if not is_round else 46
    draw_header(draw, w, header_h, "PUSHUP TIME!", bg_color=(255, 0, 0), is_round=is_round)
    
    font_numbers = get_font(30 if not is_round else 36, True)
    prog_text = "30 / 30"
    bbox2 = draw.textbbox((0, 0), prog_text, font=font_numbers)
    tw2 = bbox2[2] - bbox2[0]
    draw.text(((w - tw2)/2, header_h + 25), prog_text, fill="black", font=font_numbers)
    
    font_sub = get_font(14, True)
    sub_text = "Push-ups today"
    bbox3 = draw.textbbox((0, 0), sub_text, font=font_sub)
    tw3 = bbox3[2] - bbox3[0]
    draw.text(((w - tw3)/2, header_h + 65 if not is_round else header_h + 80), sub_text, fill=(80,80,80), font=font_sub)

def create_screenshots():
    for name, spec in platforms.items():
        is_round = spec.get("is_round", False)
        # Screen 1
        img1 = Image.new("RGB", spec["size"], spec["bg"])
        draw1 = ImageDraw.Draw(img1)
        draw_screen_1(draw1, spec["size"], is_round)
        img1.save(f"assets/store/screenshot_1_{name}.png")
        
        # Screen 2
        img2 = Image.new("RGB", spec["size"], spec["bg"])
        draw2 = ImageDraw.Draw(img2)
        draw_screen_2(draw2, spec["size"], is_round)
        img2.save(f"assets/store/screenshot_2_{name}.png")
        
        # Screen 3
        img3 = Image.new("RGB", spec["size"], spec["bg"])
        draw3 = ImageDraw.Draw(img3)
        draw_screen_3(draw3, spec["size"], is_round)
        img3.save(f"assets/store/screenshot_3_{name}.png")

def create_banner():
    try:
        img = Image.open("assets/store/raw_banner.png")
        # The generated image is likely 1024x1024 or similar.
        # We need to crop it to 720x320. Best way is to scale down width to 720, then crop height, or center crop.
        w, h = img.size
        # target ratio is 720/320 = 2.25
        target_w, target_h = 720, 320
        # Scale image so that it covers 720x320
        ratio = max(target_w / w, target_h / h)
        new_w, new_h = int(w * ratio), int(h * ratio)
        img = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        
        # Center crop
        left = (new_w - target_w) / 2
        top = (new_h - target_h) / 2
        right = (new_w + target_w) / 2
        bottom = (new_h + target_h) / 2
        img = img.crop((left, top, right, bottom))
        
        # Overlay text
        draw = ImageDraw.Draw(img)
        font_banner = get_font(52, True)
        text = "PEBBLE PUSHUP REMINDER"
        
        # Draw text with shadow for readability
        bbox = draw.textbbox((0, 0), text, font=font_banner)
        tw = bbox[2] - bbox[0]
        tx, ty = (720 - tw)/2, 80
        
        draw.text((tx+2, ty+2), text, fill="black", font=font_banner)
        draw.text((tx, ty), text, fill="white", font=font_banner)
        
        font_sub = get_font(24)
        sub_text = "Master your daily pushups."
        bbox2 = draw.textbbox((0, 0), sub_text, font=font_sub)
        tw2 = bbox2[2] - bbox2[0]
        tx2, ty2 = (720 - tw2)/2, 160
        
        draw.text((tx2+2, ty2+2), sub_text, fill="black", font=font_sub)
        draw.text((tx2, ty2), sub_text, fill="#ffdddd", font=font_sub)
        
        img.save("assets/store/banner.png")
    except Exception as e:
        print("Error creating banner:", e)

print("Generating screenshots...")
create_screenshots()
print("Generating banner...")
create_banner()
print("Done.")
