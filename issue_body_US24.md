## Description
**As a** user  
**I want to** be able to snooze reminder notifications for a specific duration  
**So that** I am not annoyed by reminders when I am away from home, busy, or otherwise unable to do pushups.

## Acceptance Criteria

### Functional Requirements
- [ ] When a reminder notification appears on the smartwatch, it must provide a "Snooze" action.
- [ ] Selecting "Snooze" must present a menu with the following duration options:
  - 1 Hour
  - 4 Hours
  - 8 Hours
  - Until Tomorrow (silences for the rest of the current day)
- [ ] While the snooze period is active, the app must not trigger any background reminders or vibrations.
- [ ] Once the selected snooze duration expires, the regular reminder schedule must automatically resume.
- [ ] The active snooze state and its expiration timestamp must be persisted so that it survives app restarts or smartwatch reboots.
- [ ] Include a way to manually cancel an active snooze from the main app menu to resume normal reminders immediately.

### Non-Functional Requirements
- [ ] The snooze interface should be easily accessible directly from the reminder prompt to minimize interactions.
- [ ] The change must seamlessly integrate with the existing background worker and wake-up timers without breaking the regular daily reset logic.
