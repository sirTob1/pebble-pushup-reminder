# US-04: Hold-to-Repeat for Fast Pushup Logging

## Description
**As a** pushup tracker user  
**I want to** hold down the Up or Down button in the pushup logging view to continuously increase or decrease the pushup count  
**So that** I can quickly log large sets of pushups without having to press the button dozens of times individually.

## Acceptance Criteria

### Functional Requirements
- [x] Pressing and holding the `UP` button in the pushup logging view must continuously and automatically increase the pushup count.
- [x] Pressing and holding the `DOWN` button in the pushup logging view must continuously and automatically decrease the pushup count.
- [x] Single clicks on the `UP` and `DOWN` buttons must continue to work normally for fine-grained (+1/-1) adjustments.
- [x] The auto-repeat interval should be balanced (e.g., 100ms) to allow for both fast counting and precise stopping at the correct target number.

### Non-Functional Requirements
- [x] **Usability & Performance:** The screen must redraw the changing pushup numbers smoothly and without lag while the button is being held down.
