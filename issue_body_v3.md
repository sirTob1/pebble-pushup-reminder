# US-14: User-defined Daily Goal as Minimum Threshold

## Description
**As a** user  
**I want to** have my configured daily goal act as the absolute minimum target  
**So that** the adaptive algorithm never reduces my goal below the baseline I consider meaningful for myself.

## Acceptance Criteria

### Functional Requirements
- [ ] The algorithm must use the user's configured daily goal as the minimum threshold instead of the hardcoded value (10).
- [ ] Any deload calculation must not reduce the effective daily goal below this user-defined baseline.
- [ ] When the user changes their daily goal in the settings (smartphone app), this should immediately serve as the new minimum for future calculations (raising the effective goal if it's below the new minimum, but leaving it if it's above).
- [ ] **Smartwatch Reset Prompt**: When the user changes their daily goal directly on the smartwatch, and their current *effective* goal is higher than the newly selected goal, the app must present a confirmation dialog asking if they want to reset their current effective goal back down to this new minimum.
- [ ] Rename the UI labels from "Tagesziel (Manuell)" / "Manual Target" to "Minimalziel" / "Minimum Goal" across the smartwatch UI and the smartphone configuration page.

### Non-Functional Requirements
- [ ] Existing persistence and UI flows should not be broken by this change.
