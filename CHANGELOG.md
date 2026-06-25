# Changelog

All notable changes to this project will be documented in this file.

## [1.3.0] - 2026-06-25

### Added
- **Hold-to-Repeat (US-04):** You can now hold down the Up or Down button in the pushup logging view to continuously increase or decrease the pushup count. This allows for much faster logging of large sets.

## [1.2.0] - 2026-06-21

### Fixed
- **Language Persistence**: Fixed an issue where changing the language in the Settings page was not correctly saved and applied due to incorrect string-to-integer conversion (US-11).

## [1.1.0] - 2026-06-17

### Added
- **Manual Override for Adaptive Goals**: Added the ability to instantly override the daily target. When you change your "Manual Target" in the settings, the app now adopts this immediately for the current day and resets the adaptive streak.
- **Main Screen Indicators**: The main menu now provides visual indicators (`[+]`, `[-]`, `[Zzz]`) next to your progress to immediately see if today's goal was scaled up, down, or if you're on a rest day.

### Changed
- **Streamlined Workflow**: Made "Quick Log" the primary action from the main menu, providing a one-click entry to log your pushups.
- **UI Rename**: Renamed "Baseline Goal" to "Manual Target" ("Tagesziel (Manuell)" in German) to clarify its purpose.

### Removed
- **Auto-Tracking Sensor Logic**: Removed the experimental accelerometer-based automatic pushup counting feature. This dramatically reduces the app size, saves battery life, and ensures users no longer encounter frustrating counting errors due to sensor inaccuracies. All tracking is now handled reliably through the Quick Log menu.

### Fixed
- **Missing Glyphs Bug**: Fixed an issue where "min" and other non-number characters were rendered as blocky placeholders across the app interface (Settings, Main Screen). All texts are now cleanly rendered using a bold system font (`BITHAM_42_BOLD`).

## [1.0.0] - Initial Release
- Basic goal setting, reminders, smartphone history export, and first stable build for all platforms.
