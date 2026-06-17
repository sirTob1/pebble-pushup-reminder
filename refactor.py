import os

file_path = "src/c/main.c"
with open(file_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

new_lines = []
skip = False

for line in lines:
    if line.startswith("// Push-up Session: Accelerometer Handler"):
        skip = True
    elif line.startswith("// ============================================================================"):
        # We need to find when the skip should end.
        pass
        
    if skip and line.startswith("// Number Picker Window"):
        skip = False
        new_lines.append("// ============================================================================\n")

    if not skip:
        new_lines.append(line)

with open(file_path, "w", encoding="utf-8") as f:
    f.writelines(new_lines)

print("Removed session handling block.")
