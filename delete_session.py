with open("src/c/main.c", "r", encoding="utf-8") as f:
    lines = f.readlines()

# 1-indexed to 0-indexed
start_idx = 616 - 1
end_idx = 996 - 1

new_lines = lines[:start_idx] + lines[end_idx + 1:]

with open("src/c/main.c", "w", encoding="utf-8") as f:
    f.writelines(new_lines)
