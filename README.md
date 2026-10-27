# Pebble Pushups ⌚💪
*A 100% Vibe Coding Project - built entirely with AI!*

Get stronger every day right from your wrist! **Pebble Pushups** is your ultimate personal trainer for the Pebble Smartwatch. Whether you are stuck at a desk or working from home, this app ensures you stay active by gently reminding you to knock out a set of pushups at custom intervals.

## 🚀 Features
- **Adaptive Daily Goals**: The app intelligently learns from your performance while always respecting your configured minimum target (5–500 pushups)! Crush your goals to trigger an 'Overload' day with a higher target, or get a 'Deload/Recovery' day if you need a break. Pushups done on a rest day also count and can trigger an 'Overload'. You can always set a new minimum goal manually!
- **Strict Mode**: Prevent your daily goal from ever being automatically reduced. For those who want to ensure their progression only moves forward, you can toggle Strict Mode in the settings.
- **Smart Reminders**: Set your active time window and reminder intervals. Reminders can automatically dismiss themselves after a configurable duration if you can't respond immediately. You can instantly **Snooze** reminders for a set duration (1h, 4h, 8h, or until tomorrow) directly from the watch, or pause them manually from the main menu at any time. The app uses the reliable Pebble Wakeup API so it doesn't drain your battery while waiting in the background. You can also log pushups directly from the reminder notification with a single button press.
- **Quick Log**: Easily and reliably log your completed pushups. Supports **Hold-to-Repeat**: hold down the up/down buttons to continuously and quickly increase or decrease the pushup count!
- **Smartphone History Dashboard**: Dive into your workout history right inside the Pebble app settings on your phone! Switch between an interactive **Bar Chart** (powered by Chart.js) and a **Data Table** view across multiple timeframes (**Last 7 Days**, **Last 30 Days**, **Last Year**, **All Time**), and export your full history as **CSV** to the clipboard.
- **Offline Sync**: Logged pushups while disconnected from your phone will automatically synchronize the next time you connect!
- **Local-First & Bilingual**: Core watch tracking, reminders, and the `pebble-clay` settings UI work offline without any external account or cloud backend (only the optional Chart.js bar diagram loads via CDN when viewing the chart tab). Available in English and German.
- **Native Watch Launcher Integration**: Includes a custom high-contrast menu icon in the Pebble app drawer across `basalt`, `chalk`, and `emery` smartwatches.

## 🛠️ Build Instructions
Since the official Pebble SDK is no longer actively maintained natively for modern OS, we use a Docker container (`rebble/pebble-sdk`) together with [`docker_build.sh`](docker_build.sh) to safely build the app and work around legacy Python 2.7 SDK issues.

1. Ensure you have [Docker](https://www.docker.com/) installed and running on your machine.
2. Clone this repository and open a terminal in the project root folder.
3. Run the build script inside the Pebble SDK container:
   ```bash
   docker run --rm -v "%cd%:/app" -w /app rebble/pebble-sdk bash /app/docker_build.sh
   ```
   *(On macOS/Linux/PowerShell, replace `%cd%` with `$(pwd)` or `${PWD}`)*
4. The compiled app will be generated at `build/app.pbw`.

## 📱 Installation (Sideloading)
1. Transfer the compiled `.pbw` file from `build/` to your smartphone (e.g., via Google Drive, Dropbox, or local file transfer).
2. Open the file on your phone using the **Pebble App** (Rebble) or **Gadgetbridge**.
3. Confirm the installation when prompted.

## 📝 Changelog
See [CHANGELOG.md](CHANGELOG.md) for the latest updates.

## 📜 License
This project is open-source. Feel free to fork and modify!
