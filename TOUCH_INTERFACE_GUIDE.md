# TouchOS Touch Interface Guide

## 🖐️ Using TouchOS with Your Acer T230H

Complete guide to using the touch-optimized package manager and interface on your custom hardware.

---

## 🎯 Touch Package Manager GUI

### **Starting the GUI**

```bash
# Command line
tpkg-touch-gui

# Or double-tap the icon on desktop
```

The interface is specifically designed for 1920x1080 touch displays with large, thumb-friendly buttons.

---

## 📱 Interface Layout

```
┌─────────────────────────────────────────────────────┐
│  TouchOS Package Manager          📦 Status         │ ← Header (100px)
├────────┬────────────────────────────────────────────┤
│        │                                            │
│  📦    │        Package List                        │
│ Inst.  │    ┌──────────────────────────────┐        │
│        │    │  vim                  v8.2    │       │
│  🌐    │    │  ✓ Installed                  │        │
│ Avail. │    │  Text editor                  │       │
│        │    └──────────────────────────────┘        │
│  🔍    │    ┌──────────────────────────────┐        │
│ Search │    │  curl                 v7.68   │       │
│        │    │  Download tool                │       │
│  🔄    │    └──────────────────────────────┘         │
│ Update │                                            │
│        │                                            │
│ 300px  │              Main Area                     │
│ Side   │                                            │
│ bar    │                                            │
│        │                                            │
│        │    ┌───────┐  ┌───────┐                   │
│        │    │Install│  │ Info  │                   │
│        │    └───────┘  └───────┘  ← 80px buttons   │
├────────┴────────────────────────────────────────────┤
│  [q][w][e][r][t][y][u][i][o][p]                    │ ← On-screen
│   [a][s][d][f][g][h][j][k][l]                      │   keyboard
│    [z][x][c][v][b][n][m]                           │   (350px)
│         [        Space        ]    [Close]          │
└─────────────────────────────────────────────────────┘
```

---

## 🎮 Touch Gestures

### **Single Touch**

#### **Tap (Quick Touch)**
- **Select package** in list
- **Press button**
- **Type on keyboard**

```
Action: Touch and release quickly
Duration: < 200ms
```

#### **Long Press**
- **Show context menu**
- **Package details**
- **Additional options**

```
Action: Touch and hold
Duration: > 500ms
Result: Context menu appears
```

#### **Drag**
- **Scroll package list**
- **Move windows**
- **Adjust sliders**

```
Action: Touch, hold, and move
Direction: Vertical for scroll
```

### **Two-Finger Gestures**

#### **Pinch to Zoom**
```
Action: Two fingers moving apart/together
Use: Zoom package details
```

#### **Two-Finger Scroll**
```
Action: Two fingers dragging up/down
Use: Fast scroll through long lists
```

#### **Two-Finger Tap**
```
Action: Quick tap with two fingers
Use: Open context menu / Back
```

---

## 🎨 Visual Design (Touch-Optimized)

### **Button Sizes**

All buttons are sized for comfortable thumb/finger tapping:

| Element | Size | Spacing |
|---------|------|---------|
| Primary buttons | 80px height | 20px margin |
| List items | 120px height | 10px gap |
| Keyboard keys | 70px height | 10px gap |
| Touch targets | Min 44px | Standard |

### **Color Coding**

- 🔵 **Blue (Header):** System/branding
- 🟢 **Green:** Installed packages
- 🔴 **Red:** Uninstall actions
- 🟡 **Yellow:** Updates available
- ⚪ **White:** Text/labels
- ⚫ **Dark gray:** Background

### **Icons & Labels**

All buttons have both icon AND text for clarity:
- 📦 Installed
- 🌐 Available
- 🔍 Search
- 🔄 Update
- ⬇️ Install
- 🗑️ Remove
- ℹ️ Info

---

## ⌨️ On-Screen Keyboard

### **Layout**

```
┌────────────────────────────────────────┐
│  [1][2][3][4][5][6][7][8][9][0][-][=]  │  ← Numbers
│                                        │
│   [q][w][e][r][t][y][u][i][o][p]      │  ← QWERTY
│                                        │
│    [a][s][d][f][g][h][j][k][l]        │  ← Home row
│                                        │
│     [z][x][c][v][b][n][m]             │  ← Bottom
│                                        │
│       [    Space Bar    ]   [Close]    │
└────────────────────────────────────────┘
```

### **Features**
- **Auto-show:** Appears when text input needed
- **Large keys:** 140px wide × 70px tall
- **Visual feedback:** Key press animation
- **Sounds:** Optional click sounds
- **Swipe typing:** Glide between letters (future)

### **Special Keys**

| Key | Function |
|-----|----------|
| Space | Space bar (1000px wide!) |
| ← Backspace | Delete character |
| ↵ Enter | Submit/new line |
| ⇧ Shift | Uppercase (toggle) |
| 123 | Numbers/symbols |
| Close | Hide keyboard |

---

## 📋 Common Tasks

### **1. Install a Package**

```
Step 1: Tap "🌐 Available" in sidebar
Step 2: Scroll to find package
Step 3: Tap package to select (highlighted)
Step 4: Tap "⬇️ Install" button
Step 5: Wait for download/install
Step 6: See "✓ Installed" badge
```

**Time:** ~10-30 seconds
**Touches:** 3 taps

### **2. Search for Package**

```
Step 1: Tap "🔍 Search" in sidebar
Step 2: On-screen keyboard appears
Step 3: Type package name
Step 4: Results filter as you type
Step 5: Tap desired package
```

**Time:** ~15-20 seconds
**Touches:** Typing + 2 taps

### **3. Remove Package**

```
Step 1: Tap "📦 Installed" in sidebar
Step 2: Tap package to select
Step 3: Tap "🗑️ Remove" button
Step 4: Confirm removal
Step 5: Package removed
```

**Time:** ~5 seconds
**Touches:** 4 taps

### **4. View Package Info**

```
Step 1: Select package (tap it)
Step 2: Tap "ℹ️ Info" button
Step 3: View details page
Step 4: Tap "← Back" to return
```

**Shows:**
- Full description
- Version
- Dependencies
- File size
- Author
- License

### **5. Update Repository**

```
Step 1: Tap "🔄 Update" in sidebar
Step 2: Wait for download
Step 3: Package list refreshes
```

**Frequency:** Once per day recommended

---

## 🎯 Touch Tips & Tricks

### **Scrolling**

**Fast Scroll:**
```
1. Two-finger drag for momentum scroll
2. Flick gesture for fast navigation
3. Tap scroll bar to jump
```

**Precise Scroll:**
```
1. Single-finger drag (slower)
2. Good for finding specific item
```

### **Selection**

**Quick Select:**
```
Single tap = Select
Double tap = Select + Open details
```

**Multi-Select (Future):**
```
Long press + drag to select multiple
```

### **Navigation**

**Back:**
```
- Swipe from left edge
- Two-finger tap
- Tap "← Back" button
```

**Home:**
```
- Three-finger swipe up
- Tap header logo
```

---

## 🔧 Customization

### **Touch Sensitivity**

```bash
# Edit /etc/touchos.conf
[Touch]
sensitivity=medium  # low, medium, high
tap_timeout=200     # ms
long_press=500      # ms
double_tap=300      # ms
```

### **Button Sizes**

```bash
# For accessibility
[UI]
button_height=100   # Default: 80
text_size=large     # small, medium, large
icon_size=48        # Default: 32
```

### **Keyboard Layout**

```bash
[Keyboard]
layout=qwerty       # qwerty, dvorak, colemak
height=350          # pixels
auto_show=true
sounds=false
```

---

## ♿ Accessibility

### **Large Text Mode**

```bash
# Settings → Accessibility → Large Text
text_multiplier=1.5
```

### **High Contrast**

```bash
# Settings → Accessibility → High Contrast
contrast=high
```

### **Touch Dwell**

```bash
# Hold finger in place instead of tapping
[Accessibility]
dwell_enabled=true
dwell_time=1000     # ms to trigger
```

### **Voice Feedback**

```bash
# Spoken descriptions (future)
[Accessibility]
speak_buttons=true
speak_packages=true
```

---

## 🐛 Troubleshooting Touch

### **Touch Not Responding**

```bash
# Check touchscreen
cat /proc/bus/input/devices | grep T230H

# Should show:
# N: Name="Quanta Optical Touch 0408:3000"
# H: Handlers=event2 mouse0
```

**Fix:**
```bash
# Restart touch driver
rmmod usb_hid
modprobe usb_hid

# Or restart system
reboot
```

### **Touch Inaccurate**

```bash
# Run calibration tool
/usr/bin/touch-calibrate

# Follow on-screen instructions:
# 1. Tap top-left corner
# 2. Tap top-right corner
# 3. Tap bottom-right corner
# 4. Tap bottom-left corner
# 5. Tap center
```

### **Multi-Touch Not Working**

```bash
# Verify multi-touch support
xinput list-props "Acer T230H"

# Should show:
# "Device Enabled": 1
# "Max Touch Points": 2
```

### **Palm Rejection Issues**

```bash
# Adjust palm detection
[Touch]
palm_rejection=true
palm_size=100       # pixels
ignore_area=50      # pixels from edge
```

---

## 📊 Performance

### **Touch Latency**

```
Hardware → Driver: ~1ms
Driver → OS: ~2ms
OS → GUI: ~5ms
Total: ~8ms (smooth!)
```

**Target:** < 20ms for good UX

### **Rendering**

```
60 FPS = 16.67ms per frame
Current: Vsync enabled
Damage tracking: Yes
Hardware accel: Partial
```

### **Optimization Tips**

1. **Reduce animations** if sluggish
2. **Disable transparency** for speed
3. **Close unused apps**
4. **Update graphics drivers**

---

## 🎨 Theme Customization

### **Color Schemes**

**Dark Mode (Default):**
```css
Background: #1E1E1E
Sidebar: #2D2D30
Header: #007ACC
Text: #FFFFFF
```

**Light Mode:**
```css
Background: #FFFFFF
Sidebar: #F3F3F3
Header: #0078D4
Text: #000000
```

**High Contrast:**
```css
Background: #000000
Text: #FFFF00
Buttons: #FFFFFF
Selected: #00FF00
```

### **Icon Packs**

```bash
# Install icon pack
tpkg install touchos-icons-flat
tpkg install touchos-icons-3d
```

---

## 📱 Mobile-Like Features

### **Pull to Refresh**

```
In package list:
Pull down from top → Release → Refreshes
```

### **Swipe Actions**

```
Swipe right on package → Quick actions
Swipe left on package → Delete
```

### **Toast Notifications**

```
Install complete: ✓ vim installed!
Error: ✗ Network unavailable
Update: ⚠ 3 updates available
```

---

## 🎓 Learning Curve

### **Beginner** (First time users)
- Time to comfort: 5 minutes
- Basics: Tap, scroll, keyboard
- Start with: Install 1-2 packages

### **Intermediate** (Regular users)
- Gestures: Pinch, swipe, long-press
- Efficiency: Keyboard shortcuts
- Customization: Themes, layout

### **Advanced** (Power users)
- CLI integration
- Scripting installs
- Custom packages

---

## 📚 Additional Resources

- **Touch Calibration:** See `HARDWARE_SETUP.md`
- **Keyboard Layout:** See `/etc/touchos/keyboard.conf`
- **Gesture Reference:** Built into Settings → Help

---

**Touch Interface: Ready to Use!** 🖐️

Your Acer T230H is fully configured for an amazing touch experience!

*Made with ❤️ for TouchOS*
*Made by Floof<3*

