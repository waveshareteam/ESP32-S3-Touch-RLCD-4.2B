# ESP32-S3-Touch-RLCD-4.2B Factory Firmware User Guide

[English](README.md) | [简体中文](README_ZH.md)

This guide describes the factory firmware built from this directory. Devices supplied with this firmware are ready to use; customers do not need to install a development environment or compile the project.

## 1. Getting Started

### What you need

- An ESP32-S3-Touch-RLCD-4.2B and a USB power cable. Flashing requires a cable that also supports data transfer.
- Ambient light: the 400×300 black-and-white reflective LCD **has no backlight**. Poor visibility in darkness does not mean the device is off.
- A FAT32 microSD card for photos or music. Other functions do not require a card.
- A compatible speaker for music, recording playback, or voice chat.
- An Internet-connected **2.4 GHz Wi-Fi** network for weather, network time, or voice chat.

### First power-on

1. Connect USB power and wait for the application desktop.
2. Tap an icon to open an app. Swipe left or right on the desktop to change pages.
3. To connect to the Internet, open **Settings → Network setup** and follow Section 3.
4. For photos or music, prepare the SD card as described in Section 4.
5. Most apps close using the upper-right back arrow. Full-screen Photos and Display Test use the gestures listed below.

The first desktop page contains Drawing, AIChats, Weather, Music, 2048, Photos, Sensor, and Settings. The second contains Button Test, Display Test, and Mic Test.

## 2. Desktop and Navigation

| Action or indicator | Meaning |
| --- | --- |
| Tap a desktop icon | Open the app |
| Swipe left/right on the desktop | Change app pages |
| App back arrow | Usually at the upper right; Drawing has Back at the upper left |
| Quick upward swipe from the bottom edge | Return to the desktop in most apps; an app may pause rather than close. Use an audio app's own back button to release its resources |
| Full-screen Photos | Swipe upward from the middle of the image to return to the file list, then tap the upper-right arrow to exit |
| Display Test | Double-tap anywhere to exit, with approximately 400 ms or less between taps; system navigation gestures are disabled in this app |
| Custom button on the desktop | Change the desktop expression |
| Wi-Fi icon | Connection status, not signal strength |
| Battery `--%` | No valid battery reading yet, or cached data has expired |

The desktop shows `--:--` until the first successful network time synchronization of the current boot. It then displays Beijing time (UTC+8). The system clock continues running after a disconnection; a power cycle requires another network synchronization. **The desktop clock and the RTC factory test in Sensor use separate time sources.**

## 3. Connecting to Wi-Fi

### Recommended: configure using a phone

1. Open **Settings → Network setup** on the device and keep this page open throughout setup.
2. Scan the QR code with your phone to join the device's `RLCD4.2-xxxxxx` hotspot. Its password is `12345678`. The QR code joins the device hotspot; it does not directly open the router credentials form.
3. If the phone reports that this network has no Internet, choose to stay connected. Do not switch to another Wi-Fi network yet.
4. If the setup page does not open automatically, enter **http://192.168.4.1** in a browser, using HTTP rather than HTTPS. The phone must still be connected to the device hotspot.
5. Enter the target 2.4 GHz network in **Network name (SSID)** and **Password**, then tap **Connect and Save**.
6. Wait for **Connected and saved**. Credentials are stored, and the device hotspot closes about 5 seconds after success. Your phone can reconnect to its usual network.
7. Return to the desktop and wait for time synchronization, or open Weather or AIChats.

The device prioritizes the saved network on subsequent boots. Repeat setup to change networks. If the page reports a successful connection but failed saving, the connection may work now but require setup again after a restart.

**Settings → Wi-Fi only scans nearby networks**; tapping a scanned network does not connect to it. A normal connection failure does not automatically start the setup hotspot. Open Network setup manually.

### Using the preset test hotspot

When no saved credentials exist, the firmware tries this test network:

| Setting | Value |
| --- | --- |
| SSID | `weather-wifi` |
| Password | `12345678` |
| Band | 2.4 GHz |

Enable mobile data when using a phone hotspot, and select 2.4 GHz or a compatibility mode where available. **Saved credentials take priority over this preset.** There is no need to rename your router for normal use.

A Wi-Fi connection alone does not guarantee Internet access. Time synchronization, weather, and voice chat also require access to their servers. Drawing, photos, and local hardware tests work offline.

## 4. Preparing the SD Card

Use **FAT32**; exFAT is not supported by the current configuration. Power off before inserting or removing the card, and do not remove it while files are being read.

At each boot, the device performs one background SD card and capacity check. Open **Settings → SD Card** to see the result without first pressing a check button. Wait if Checking is displayed. After inserting or replacing a card, or changing its contents, tap **Check card** to refresh. This is not continuous hot-plug detection; there are no format or eject controls.

Create these folders at the card root. Place files directly inside them, without additional subfolders:

```text
SD card root/
├── photo/
│   ├── 01_landscape.jpg
│   └── 02_cat.jpeg
└── music/
    ├── 01_song.mp3
    └── 02_song.mp3
```

Use English letters, numbers, and underscores where possible. Numeric prefixes control filename sorting. Some non-English characters may not display correctly.

### Photo requirements

| Item | Requirement |
| --- | --- |
| Folder | `photo` at the SD card root |
| Format | Baseline JPEG, `.jpg` or `.jpeg`, case-insensitive |
| Encoding | 8-bit RGB, YCbCr, or grayscale JPEG |
| Recommended resolution | 400×300 |
| Maximum file size | 2 MiB |
| Maximum pixel count | Width × height must not exceed 2,097,152 pixels |
| Maximum file count | 128 photos |
| Unsupported | Progressive JPEG, CMYK JPEG, PNG, BMP, GIF, HEIC |

Other aspect ratios are fitted with white margins, without stretching or cropping. EXIF orientation is not applied automatically; rotate the image on a computer and export it before copying it to the card.

The display has only black and white pixels. Color and grayscale photos are converted using dithering, which produces visible dots. Preparing high-contrast 400×300 black-and-white JPG files gives more control over the result.

Copy the supplied [sample photos](components/Photos/sdcard_example/photo/) into the card's `photo` folder for a quick test.

### Music requirements

Place actual MP3-encoded `.mp3` files in the `music` folder at the card root. Renaming a file extension does not convert its format. Subfolders, WAV, and FLAC are not supported.

## 5. Application Guide

| App | Purpose | Additional requirements |
| --- | --- | --- |
| Drawing | Monochrome touch drawing | None |
| AIChats | Xiaozhi voice chat | Internet, service activation, speaker |
| Weather | Weather information | Internet |
| Music | MP3 playback | SD card, speaker |
| 2048 | Number merging game | None |
| Photos | Full-screen still image viewer | SD card with JPEG files |
| Sensor | RTC, temperature, humidity | Onboard sensors |
| Settings | Volume, network, SD card, device information | Depends on the function |
| Button Test | Button checks | Onboard Custom and BOOT buttons |
| Display Test | All-black/all-white screen checks | None |
| Mic Test | Dual microphone levels, recording, playback | Speaker for playback |

### Drawing

Touch and drag inside the canvas to draw with a fixed black brush. Tap **Clear** at the upper right to erase the canvas, or **Back** at the upper left to exit. There are no eraser, undo, or SD save controls. Reopening the app clears the canvas.

### AIChats

1. Connect to the Internet before opening AIChats.
2. On an unbound device, follow the on-screen instructions to add the device in the Xiaozhi console using its activation code.
3. When ready, say “**你好小智**” (Ni hao Xiao Zhi), or tap **Talk** to start a conversation.
4. Finish speaking and wait for recognition and the reply. **Stop** cancels the current interaction; the wake word can also interrupt a reply.
5. Scroll the conversation text vertically. Use the upper-right back arrow to exit.

The app uses the shared Settings volume; 0 is silent. Exiting stops wake-word listening and releases audio resources. If a network or service error occurs, restore connectivity and follow the page status while it reconnects. A complete firmware flash must include the speech model; flashing only the application BIN does not provide all wake-word resources.

### Weather

Opening the app automatically requests weather and shows the city, update time, conditions, temperature, humidity, wind speed, and wind direction. The default city is **Shenzhen**, not a location automatically determined by GPS or Wi-Fi.

Tap the weather illustration to refresh manually. Successful updates repeat about every 5 minutes; failed requests retry after about 30 seconds. Previously fetched data from the current boot may remain visible offline, so check the timestamp and status. Use the upper-right back arrow to exit.

Developers can change the default city under `menuconfig → RLCD Weather` and rebuild.

### Music

1. Prepare the SD card as described in Section 4 and open Music. The app scans files and starts paused.
2. Use Play/Pause to control playback. Resuming after a pause continues from the current position.
3. Previous/Next cycle through tracks while retaining the playing or paused state.
4. Playback stops at the end of a track; it does not automatically advance. Press Play again to restart that track.
5. Exit using the upper-right back arrow or an upward swipe from the middle of the page. Closing the app stops playback.

The progress bar is read-only and does not support seeking. **Music Player / Chill Vibes** are fixed cover text, not the current filename or track metadata. After a read error, check the card and files, press the center button to rescan, then press Play.

### 2048

Swipe up, down, left, or right inside the board to merge matching numbers and increase your score. **New Game** restarts the game; the result page also allows restarting after winning or running out of moves. The best score lasts for the current powered session; neither it nor game progress is guaranteed to survive a restart. Use the upper-right back arrow to exit.

Start swipes inside the board to avoid triggering navigation gestures at the screen edges.

### Photos

1. Open Photos and wait for the `photo` folder scan and file list.
2. Tap a filename to show the image full-screen, without the app header or status bar.
3. Swipe left from the middle of the image for the next photo, or right for the previous one. Navigation wraps at the ends.
4. **Swipe upward from the middle of the image to return to the list**, then tap the upper-right back arrow to exit.
5. Use **Rescan** after changing files to refresh the list and image cache.

This is a manually controlled still image viewer. There is no timed slideshow, GIF animation, or web upload. The app does not create, delete, or modify photos on the SD card. The startup SD check does not decode all photos; images are read and decoded when selected.

### Sensor

Displays the PCF85063 RTC and SHTC3 temperature/humidity readings. The RTC refreshes about once per second and temperature/humidity about every 5 seconds. Invalid readings use placeholders.

**This is a separate RTC factory test.** RTC initialization writes the test starting time **2026-04-22 23:59:00**; reopening or resuming the page may initialize it again. This is not Beijing time and is not synchronized with the desktop network clock. Use the upper-right back arrow to exit.

### Settings

- **Volume**: shared volume from 0–100 in steps of 5, default 55. Changes apply immediately and save when the slider is released, surviving a restart. 0 mutes playback. Adjust again if saving fails.
- **Wi-Fi**: scan nearby 2.4 GHz networks and inspect names, signal, channels, and security. This page does not connect by selecting a network.
- **Network setup**: QR-assisted web provisioning, described in Section 3.
- **SD Card**: startup mount status, total capacity, and free space. Check card refreshes the result. MiB/GiB use binary units.
- **About**: firmware version, ESP-IDF, MAC, Flash, PSRAM, and display information.

The back arrow returns from a subpage to the Settings home page; tap again to exit. Raising the volume does not automatically resume paused music.

### Button Test

Press and release **Custom (GPIO18)** and **BOOT (GPIO0)**. The page shows PRESSED/RELEASED states and counts; a completed test shows PASS for that button, and 2/2 when both pass. The refresh icon clears results. Use the upper-right back arrow to exit.

Test BOOT after normal startup. Holding it during power-on or reset can enter download mode.

### Display Test

The app starts with black and repeatedly displays **all black for 10 seconds → all white for 5 seconds** to inspect uniformity and pixel defects. There are no text or header elements; the black interval is intentional.

**Double-tap anywhere to exit**, with approximately 400 ms or less between taps. System navigation gestures are disabled on this page.

### Mic Test

1. Observe the live MIC1 and MIC2 levels while speaking near each microphone.
2. Tap **Record** to record both channels for up to about 5 seconds. **Stop** ends recording early.
3. Use the **MIC1 / MIC2** playback buttons to listen to each recorded channel. Stop ends playback.
4. A new recording replaces the previous one. The upper-right back arrow exits and releases the recording resources.

Recordings stay in memory and are not saved to the SD card. dBFS measures digital audio level, not acoustic sound pressure in dB SPL. CLIPPED indicates a signal near full scale; move farther away or speak more quietly and record again. Both microphones picking up sound is normal.

Music, AIChats, and Mic Test share audio hardware. Exit through an app's own back button before switching. If playback is silent, check the speaker connection and Settings volume.

## 6. Understanding the Battery Reading

The desktop percentage is an **estimate of remaining charge based on sampled voltage**. It is not a precise capacity measurement from a dedicated fuel gauge and does not predict exact remaining runtime.

The firmware forms a sample window about every 2 seconds, then applies outlier removal, filtering, hysteresis, and display rate limiting. The voltage mapping spans approximately 3.00–4.12 V. The percentage normally changes gradually, with faster decreases allowed at low voltage. A percentage calculated directly from an instantaneous voltage can therefore differ from the displayed value.

Charging, load, and battery condition affect the reading. The firmware does not reliably detect battery presence or charging state and has no on-screen charging animation. Refer to the board's charging indicator for charging status.

## 7. Troubleshooting

| Symptom | What to do |
| --- | --- |
| Screen is hard to read in darkness | Increase ambient lighting; this reflective LCD has no backlight |
| Desktop time is `--:--` | Configure Wi-Fi and wait for the first Internet time synchronization of this boot |
| Wi-Fi connects but weather/time does not update | Check Internet access and access to the relevant servers |
| Phone cannot open 192.168.4.1 | Keep Network setup open, join the device hotspot, use HTTP, and prevent automatic network switching |
| A scanned Wi-Fi network cannot be selected | The scan page is informational; use Network setup |
| Preset hotspot does not connect after a change | Saved credentials take priority; update them through Network setup |
| SD card is not detected | Check insertion and FAT32 formatting, then tap Settings → SD Card → Check card |
| Photo list is empty | Check the root `photo` folder, extensions, and JPEG encoding, then tap Rescan |
| Photo fails to open or is rotated | Re-export baseline JPEG within the limits in Section 4; rotate it on a computer first |
| Photos show dotted patterns | This is monochrome dithering; try preprocessed high-contrast black-and-white images |
| Full-screen photo has no back button | Swipe upward from the middle of the image to return to the list |
| Display Test looks stuck on black | Wait 10 seconds for white, or double-tap to exit; check ambient light |
| Music or recording playback is silent | Check speaker wiring and shared volume; close other audio apps |
| Music does not advance at track end | Playback intentionally stops; select the next track manually |
| Sensor time differs from the desktop | Sensor uses the independent RTC factory test; the desktop uses network time |
| Battery changes slowly or differs from a voltage calculation | Filtering and rate limiting apply to a voltage-based estimate |
| Voice chat asks for activation | Complete Xiaozhi device binding using the on-screen instructions and an Internet connection |

## 8. Reflashing and Developer Notes

Preinstalled firmware does not require compilation. If you have obtained a **complete merged BIN** for this product, flash it at **0x0** using ESP32-S3, 16 MB Flash, DIO mode, and 80 MHz. Erasing or writing a full image may clear saved Wi-Fi, volume, and device activation information. It does not delete SD card files.

The source project uses **ESP-IDF v6.0.2 and LVGL v9.5.0**. From an ESP-IDF terminal in this directory, run:

```sh
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Press `Ctrl+]` to exit the serial monitor. Use the generated project flash parameters to include speech model and other partitions; `build/flash_args` is authoritative. `build/esp32_s3_touch_rlcd_4_2b.bin` is an **application partition image**, not a complete merged firmware, and must not be flashed at 0x0.

The shared BSP is [esp32_s3_touch_rlcd_4_2b](components/esp32_s3_touch_rlcd_4_2b/). See the [Photos notes](components/Photos/README.md), [Weather notes](components/Weather/README.md), [Settings notes](components/Settings/README.md), [Music notes](components/Music/README.md), and [Mic Test notes](components/MicTest/README.md) for implementation details.
