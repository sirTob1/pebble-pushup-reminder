# US-1: Configure Daily Goal and Reminder Schedule

## Description
**As a** user  
**I want to** set a daily push-up target, a reminder interval, and active hours for reminders  
**So that** I can tailor the reminder routine to my daily schedule and fitness goals.

## Acceptance Criteria

### Functional Requirements
- [ ] The user can set a daily target number of push-ups (e.g., via a settings menu).
- [ ] The user can set a reminder interval (e.g., every 30 minutes, 1 hour, 2 hours).
- [ ] The user can configure the active time window with a start time and an end time (e.g., 08:00 to 20:00).
- [ ] All settings must be persisted on the watch (survive app closing/restarts).

### Non-Functional Requirements
- [ ] The configuration menu must be easy to navigate on the Pebble screen using the hardware buttons.
- [ ] Time and number picker inputs should feel responsive and include boundary checks (e.g., no negative numbers, end time must be after start time).
