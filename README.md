# Pebble Pushups ⌚💪
*A 100% Vibe Coding Project - built entirely with AI!*

Get stronger every day right from your wrist! **Pebble Pushups** is your ultimate personal trainer for the Pebble Smartwatch. Whether you are stuck at a desk or working from home, this app ensures you stay active by gently reminding you to knock out a set of pushups at custom intervals.

## 🚀 Features
- **Adaptive Daily Goals**: The app intelligently learns from your performance! Crush your goals to trigger an 'Overload' day with a higher target, or get a 'Deload/Recovery' day if you need a break. You can always override the target manually!
- **Smart Reminders**: Set your active time window and reminder intervals. The app uses the reliable Pebble Wakeup API so it doesn't drain your battery while waiting in the background.
- **Quick Log**: Easily and reliably log your completed pushups with a few clicks.
- **Smartphone Dashboard**: Dive into your workout history! Check out a beautiful 14-day performance chart right inside the Pebble app settings on your phone, and export your data as CSV.
- **Fully Offline & Bilingual**: No cloud dependencies! The settings menu (`pebble-clay`) renders completely offline. Available in English and German.

## 🛠️ Build Instructions
Since the official Pebble SDK is no longer actively maintained natively for modern OS, we use a Docker container to build the app safely.

1. Ensure you have [Docker](https://www.docker.com/) installed on your machine.
2. Clone this repository.
3. Open your terminal/command prompt in the project root folder.
4. Run the Pebble SDK build command via Docker:
   ```bash
   docker run --rm -v "%cd%:/app" -w /app rebble/pebble-sdk pebble build
   ```
   *(On macOS/Linux, replace `%cd%` with `$(pwd)`)*
5. The compiled app will be generated at `build/app.pbw`.

## 📱 Installation (Sideloading)
1. Transfer the `build/app.pbw` file to your smartphone (e.g., via Google Drive, Dropbox, or Email).
2. Open the file on your phone using the official **Pebble App** (or the Rebble patched app).
3. Confirm the installation when prompted.

## 📝 Changelog
See [CHANGELOG.md](CHANGELOG.md) for the latest updates.

## 📜 License
This project is open-source. Feel free to fork and modify!
