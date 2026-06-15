# US-2: Periodic Push-up Reminders

## Description
**As a** user  
**I want the app** to alert me to do push-ups at my configured interval during my active hours, as long as I haven't reached my daily goal  
**So that** I stay active and achieve my daily target without having to remember it myself.

## Acceptance Criteria

### Functional Requirements
- [ ] The app schedules reminders according to the configured interval (e.g., using Pebble's Wakeup API or background worker).
- [ ] Reminders are only triggered if the current time is within the configured start and end times.
- [ ] Reminders are suppressed/stopped for the day as soon as the tracked push-up count meets or exceeds the daily target.
- [ ] Triggering a reminder alerts the user with a distinct vibration pattern and a screen prompt.

### Non-Functional Requirements
- [ ] Background scheduling must be highly energy-efficient to minimize battery consumption on the watch.
- [ ] The reminder screen/vibration must be dismissible with a single button press.
