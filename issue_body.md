# US-14: User-defined Daily Goal as Minimum Threshold

## Description
**As a** user  
**I want to** have my configured daily goal act as the absolute minimum target  
**So that** the adaptive algorithm never reduces my goal below the baseline I consider meaningful for myself.

## Acceptance Criteria

### Functional Requirements
- [ ] The algorithm must use the user's configured daily goal as the minimum threshold instead of the hardcoded value (10).
- [ ] Any deload calculation must not reduce the effective daily goal below this user-defined baseline.
- [ ] When the user changes their daily goal in the settings, this should immediately serve as the new minimum for future calculations.

### Non-Functional Requirements
- [ ] Existing persistence and UI flows should not be broken by this change.
