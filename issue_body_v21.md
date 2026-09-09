# US-21: Fix Empty History on Settings Page (WebView LocalStorage Isolation)

## Description
**As a** user
**I want to** see my actual pushup history and chart on the Settings Page
**So that** I can track my progress visually and export my data.

**Bug Analysis:**
Currently, the settings page always displays "No history available yet." This happens because `index.js` (PebbleKit JS) saves the history into its own background `localStorage`, but the configuration page (`custom-clay.js`) runs inside a WebView which has an isolated, empty `localStorage`. 

## Acceptance Criteria

### Functional Requirements
- [ ] Pass the history data from `index.js` to the Clay WebView (e.g., using `clay.meta.userData` or URL parameters) before generating the configuration URL.
- [ ] Update `custom-clay.js` to read the history data from the injected `userData` instead of querying the empty WebView `localStorage`.
- [ ] The Settings Page must correctly display the 7-day bar chart, the total entries count, and allow CSV export of the actual data.

### Non-Functional Requirements
- [ ] The fix must be compatible with standard Pebble configurations as well as third-party apps like Gadgetbridge.
- [ ] No existing history data should be lost or overwritten during this fix.
