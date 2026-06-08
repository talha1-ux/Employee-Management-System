# AdvancedEMS — Setup Guide

## Prerequisites
- Qt 6.x with Qt Creator
- Qt Multimedia module (included with standard Qt6 install)
- A webcam (built-in or USB)

## NO external libraries needed
This project uses ONLY Qt6 built-in modules:
- Qt Widgets, Qt Multimedia, Qt MultimediaWidgets, Qt PrintSupport
- No OpenCV, no extra installs, no DLLs to copy

---

## Step 1 — Check Qt Multimedia is installed

In Qt Creator: Help → About Plugins → verify "Multimedia" is listed.
Or in Qt Maintenance Tool, ensure "Qt Multimedia" is ticked for your Qt version.

---

## Step 2 — Build

Open `AdvancedEMS.pro` in Qt Creator → Click Build (Ctrl+B).

---

## How Face ID Works

### Enrollment
1. Click "Enroll Face ID" when adding an employee
2. Live camera feed opens in the dialog
3. A green box appears when a face is detected
4. Click "Start Face Scan" — after 2 seconds the face is captured
5. Encoded as Base64 JPEG and saved with the employee record

### Verification
1. Select employee → Face ID Verify
2. Live camera opens
3. Face is compared against stored template using RGB histogram
   Chi-square distance. Score ≤ 0.40 = match confirmed.

### Tuning sensitivity
In `FaceIdDialog.cpp`, adjust:
```cpp
static constexpr double MATCH_THRESHOLD = 0.40;
// Lower = stricter (may reject same person in different light)
// Higher = looser  (may accept different people)
// Recommended range: 0.30 – 0.55
```

---

## Fingerprint (ZKTeco hardware)
Stub calls are in `FingerprintDialog.cpp`. Download ZKFinger SDK
from zkteco.com and uncomment the SDK lines to go live.
