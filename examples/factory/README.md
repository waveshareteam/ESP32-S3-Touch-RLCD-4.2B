# ESP32-S3-Touch-RLCD-4.2B Factory Firmware User Guide

[English](README.md) | [简体中文](README_ZH.md)

Connect USB power to try 11 apps, including drawing, photos, music, voice chat, and hardware tests. This guide starts with your first power-on and explains how to use each app.

## 1. Try This First

1. Place the board somewhere well lit, then connect USB power. The screen **has no backlight** and needs ambient light to be readable.
2. Wait for the app icons to appear. Tap **Drawing**, then draw a few lines with your finger in the blank area.
3. Tap **Clear** at the upper right. The canvas should become blank. Tap **Back** at the upper left to return to the desktop.
4. Swipe left on the desktop to find **Button Test** on the second page. Press and release the board's Custom and BOOT buttons. Both items should show PASS.
5. Tap the upper-right back arrow to return to the desktop. You can now connect to Wi-Fi or try another offline app.

**If you cannot find an app, swipe left or right on the desktop.** Page one contains Drawing, AIChats, Weather, Music, 2048, Photos, Sensor, and Settings. Page two contains Button Test, Display Test, and Mic Test.

### What do you need for each activity?

| What you want to try | What to prepare |
| --- | --- |
| Drawing, 2048, screen/button tests, temperature and humidity | Just connect power |
| Automatic Beijing time and weather | An Internet-connected 2.4 GHz Wi-Fi network |
| Voice chat | Internet-connected 2.4 GHz Wi-Fi, a compatible speaker, and Xiaozhi device binding on first use |
| Photos | A FAT32 microSD card with JPG files |
| Music | A FAT32 microSD card with MP3 files and a compatible speaker |
| Microphone recording and playback | A compatible speaker; recording does not need an SD card |

### How do you get back to the desktop?

- **Most apps**: tap the upper-right back arrow. Drawing has Back at the upper left.
- **Full-screen photo**: swipe upward from the middle of the image to return to the file list, then tap the upper-right back arrow.
- **Display Test**: tap the screen twice in quick succession to exit.
- A quick upward swipe from the very bottom edge also returns to the desktop in most apps. After music, voice chat, or recording, use the app's own back button before opening another app.

## 2. Connect the Board to Wi-Fi

Once connected, the desktop can automatically show Beijing time, Weather can fetch updates, and AIChats can connect to the voice service.

### Set up the network using your phone

1. On the board, tap **Settings**, then **Network setup**. Keep this page open until setup is complete.
2. Scan the on-screen QR code with your phone to join the board's Wi-Fi hotspot. Its name looks like **RLCD4.2-xxxxxx**, and the password is **12345678**. You can also find it manually in your phone's Wi-Fi settings.
3. If your phone says this network has no Internet, choose **Stay connected** or the equivalent option. This is normal: you are connecting your phone to the board at this step.
4. A setup web page may open automatically. Otherwise, open a browser and enter **http://192.168.4.1** in its **address bar**.
5. Enter the details of your router or another phone's hotspot:

   | Web page field | What to enter or do |
   | --- | --- |
   | Network name (SSID) | The Wi-Fi name you want the board to join |
   | Password | That Wi-Fi network's password |
   | Connect and Save | Tap this button after filling in both fields |

6. Wait for **Connected and saved**. Setup is complete. About 5 seconds later, the board turns off its setup hotspot, so your phone disconnecting from it is normal.
7. Return to the board's desktop and wait for the time to update. On later boots, the board automatically connects to the network you saved.

**Use 2.4 GHz Wi-Fi; 5 GHz networks are not supported.** Repeat these steps after changing your router or password.

If the page will not open, check that the board still shows Network setup, your phone is still connected to the board's hotspot, and you entered `http://192.168.4.1` rather than HTTPS or a search query.

The **Wi-Fi** page in Settings only lists nearby networks. **Use Network setup to make a connection.**

### Alternative: use the preset hotspot

If you have not saved a Wi-Fi network yet, you can enable a 2.4 GHz phone hotspot named `weather-wifi`, with password `12345678`, and turn on mobile data. The board will try to connect to it.

Saved networks take priority over this preset. If a different network is already saved, use Network setup to change it.

### Is the clock working?

- `--:--` after startup: network time has not arrived yet. Connect to the Internet and wait.
- After a successful update: the desktop shows Beijing time (UTC+8).
- Wi-Fi disconnects afterward: the clock keeps running.
- Power is disconnected and restored: the board needs another network time update.

## 3. Add Your Photos and Music

### Prepare the SD card

1. Use a **FAT32** microSD card. Back up existing files before formatting, because formatting deletes them. This firmware does not support exFAT.
2. Connect the card to a computer using a card reader. Open the card and create two folders directly inside it: **photo** and **music**. This first level is called the card root; do not put these folders inside another folder.
3. Put photos in photo and music in music. For example:

   ```text
   SD card/
   ├── photo/
   │   ├── 01_cat.jpg
   │   └── 02_landscape.jpg
   └── music/
       ├── 01_song.mp3
       └── 02_song.mp3
   ```

4. Safely eject the card from the computer. Turn off the board, insert the card, then power on again.
5. Open **Settings → SD Card** on the board. The board checks the card automatically at startup. If you see **Checking...**, wait for the capacity to appear. If it is not detected, check insertion and formatting, then tap **Check card**.

### Start with the sample photos

Copy the JPG files from the [sample photo folder](components/Photos/sdcard_example/photo/) into the card's photo folder, then open Photos on the board. Confirm that these work before trying your own pictures; this makes troubleshooting easier.

Prepare your own photos as follows:

- Resize them to **400×300 pixels** in an image editor and export as ordinary **JPG/JPEG**. Rotate the picture correctly before exporting; the board does not automatically correct photo orientation.
- If the export options include **Progressive**, turn it off. Use RGB or grayscale color, rather than CMYK.
- Keep each file within **2 MiB**, with no more than **128 photos**. Larger original images must also have width × height no greater than **2,097,152 pixels**.
- Use filenames such as `01_cat.jpg`, with English letters, numbers, and underscores, for easier display and sorting.
- Convert PNG, GIF, HEIC, and other formats to JPG properly. Renaming the extension to `.jpg` does not convert a file.

This is a black-and-white screen. Color photos can be opened, but they appear in black and white, with dotted patterns in shaded areas. For clearer outlines, try preparing high-contrast black-and-white photos in advance. Images with other aspect ratios have white margins to avoid distortion.

### Prepare music files

Put actual **MP3** files directly in the music folder, with names such as `01_song.mp3` and `02_song.mp3`. Do not use subfolders. Convert WAV, FLAC, or other audio formats to MP3 first.

## 4. Use Each App

### Drawing: draw on the screen

1. Open **Drawing** and drag your finger across the blank area to draw black lines.
2. Tap **Clear** at the upper right to erase the whole canvas.
3. Tap **Back** at the upper left to return to the desktop.

Drawings are temporary and are not saved. Reopening the app gives you a blank canvas.

### AIChats: talk to Xiaozhi

1. Connect to Wi-Fi as described in Section 2, check that the speaker is connected, and open **AIChats**.
2. If an activation code appears, follow the on-screen instructions to add the device in the Xiaozhi console and enter the code. After binding, wait for the device to become ready.
3. Say “**你好小智**” (Ni hao Xiao Zhi), or tap **Talk**, then ask a question such as “What day is it today?”
4. Finish speaking and wait. Conversation text appears on the screen, and the speaker plays the reply. Tap **Stop** to end the interaction, or use the wake word to interrupt a reply.
5. Tap the upper-right back arrow to exit. The board stops listening for the wake word after you leave the app.

If you cannot hear a reply, raise the Settings volume, make sure it is not 0, and check the speaker. For network errors, check that your Wi-Fi has Internet access.

### Weather: check the weather

1. Connect to the Internet, open **Weather**, and wait for weather, temperature, humidity, and wind information.
2. Tap the **weather illustration** to update immediately. The page also normally updates about every 5 minutes.
3. Tap the upper-right back arrow to exit.

The default city is **Shenzhen**; it does not automatically follow your location. Old readings may stay on the page after disconnection, so check the update time. To change the city, see the developer notes at the end.

### Music: play a song

1. Add MP3 files as described in Section 3, connect the speaker, and open **Music**.
2. Tap the center **Play button** to start. Tap again to pause, and again to resume.
3. Use Previous/Next to change songs. Files are sorted by name.
4. Tap the upper-right back arrow to exit and stop playback. An upward swipe from the middle of the page also exits.

Playback stops when a song ends; select the next song manually. The progress bar shows your position but cannot be dragged. Music Player / Chill Vibes are decorative text, not the current song title.

### 2048: merge matching numbers

1. Open **2048** and swipe up, down, left, or right inside the board to move the tiles.
2. Matching numbers merge: two 2 tiles become a 4. Keep going and try to reach 2048.
3. Tap **New Game** to restart. You can also restart from the result page.
4. Tap the upper-right back arrow to exit.

Game progress is not saved automatically. The best score lasts for the current powered session. Swipe inside the board rather than from screen edges to avoid triggering navigation.

### Photos: view pictures full-screen

1. Add JPG files as described in Section 3, open **Photos**, and wait for the file list.
2. Tap a filename to show the picture full-screen. There is no top bar or back button in this view.
3. **Swipe left from the middle of the image for the next photo, or right for the previous one.** Navigation wraps at the ends.
4. **Swipe upward from the middle of the image to return to the list**, then tap the upper-right back arrow to exit.
5. After changing photos on the card, tap **Rescan** on the list page before opening them again.

Change photos by swiping; there is no automatic slideshow. The app only reads the pictures and does not delete or modify the originals.

### Sensor: check temperature, humidity, and test time

1. Open **Sensor** to see temperature, humidity, and a clock.
2. Temperature and humidity update about every 5 seconds, and the clock changes every second. If placeholders appear at first, wait a moment.
3. Tap the upper-right back arrow to exit.

**This clock tests the board's RTC chip; it is not the desktop's Beijing time.** The test starts at **2026-04-22 23:59:00** and may restart when the app is reopened or resumed. For actual Beijing time, use the desktop after connecting to the Internet.

### Settings: volume, Wi-Fi, and SD card checks

Open **Settings**, then choose what you need:

| On-screen name | What to do |
| --- | --- |
| Volume | Drag right for louder sound. 0 is mute. The setting saves when you release the slider and is restored at the next boot |
| Wi-Fi | View nearby Wi-Fi names and signal information |
| Network setup | Connect the board to your Wi-Fi; follow Section 2 |
| SD Card | Check detection, total capacity, and free space. Tap Check card to check again |
| About | View firmware version and device information |

The back arrow first returns from a subpage to the Settings home page; tap again for the desktop. After raising the volume, return to music or recording playback and tap Play to start the sound.

### Button Test: check the physical buttons

1. Open **Button Test**, then press and release **Custom** on the board. Watch the state and count change.
2. Press and release **BOOT**. Each completed check shows **PASS**; both completed checks show **2/2**.
3. Tap the refresh icon to clear the results and test again. The upper-right back arrow exits.

Wait for normal startup before testing BOOT. Holding BOOT while powering on may enter download mode instead of showing the desktop.

### Display Test: check black and white pixels

1. Open **Display Test**. The screen stays **all black for 10 seconds**, then **all white for 5 seconds**, repeating continuously.
2. In good lighting, look for pixels that stay unchanged or areas that appear uneven.
3. To exit, **tap the screen twice in quick succession**. If it does not exit, try two taps closer together.

This page intentionally has no text or back button. When it first appears black, wait the full 10 seconds before assuming something is wrong.

### Mic Test: record and listen back

1. Connect the speaker and open **Mic Test**. Speak toward the board; the MIC1 and MIC2 level bars should respond.
2. Tap **Record** and say “This is a microphone test.” Wait about 5 seconds for recording to finish, or tap **Stop** to finish early.
3. Tap the **MIC1** and **MIC2** playback buttons separately to hear what each microphone recorded.
4. Tap Record again to replace the previous recording. The upper-right back arrow exits.

The recording is for immediate listening and is not saved to the SD card. If it is silent, check the speaker and Settings volume. If **CLIPPED** appears, move farther away or speak more quietly and record again; the sound was too loud and may be distorted.

## 5. Read the Battery Percentage

The desktop percentage **estimates remaining charge from battery voltage**. Use it as a rough indication, rather than an exact capacity or remaining runtime. If it shows `--%` just after startup, wait a few seconds.

To keep the reading stable, the percentage does not follow every voltage fluctuation immediately. Charging or playing sound can also affect it. Check the board's charging indicator to see whether it is charging; there is no on-screen charging animation.

## 6. If Something Does Not Work

| What you see | Try this first |
| --- | --- |
| Screen is hard to read in darkness | Move to better lighting; this screen has no backlight |
| Desktop keeps showing `--:--` | Follow Section 2, check Internet access, and wait for a time update |
| Phone says the board hotspot has no Internet | Stay connected and open http://192.168.4.1 in a browser |
| Setup page will not open | Keep Network setup open and check that the phone has not switched to another Wi-Fi network |
| Tapping a Wi-Fi name does nothing | This is the scan page; go back and open Network setup |
| Web page says connected but saving failed | Repeat setup; successful saving is needed for automatic connection after restarting |
| Photo or music list is empty | Check Settings → SD Card, then check that photo/music are directly at the card root |
| SD Card shows no capacity | Check insertion and FAT32 format, then tap Check card |
| Your photo will not open | Try the sample images in Section 3, then re-export your photo as an ordinary 400×300 JPG |
| Photo is rotated incorrectly | Rotate and re-export it on a computer, copy it again, and tap Rescan |
| Photo contains many tiny dots | Black-and-white conversion produces this pattern; try a high-contrast black-and-white image |
| You cannot leave a full-screen photo | Swipe upward from the middle, then tap the back arrow in the list |
| Screen test is black with no text | Wait 10 seconds for white, or double-tap quickly to exit |
| Music or recorded playback is silent | Check the speaker, raise Settings volume, and exit other audio apps using their back buttons |
| Weather is out of date | Check the network, tap the weather illustration, and check the update time |
| Sensor and desktop clocks differ | Sensor shows the RTC test clock; the desktop shows network-synchronized Beijing time |

## 7. Modify or Reflash the Firmware

For normal use, follow the sections above. For development, this directory contains the factory source project, using **ESP-IDF v6.0.2 and LVGL v9.5.0**.

Open an ESP-IDF terminal in this directory and run:

```sh
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Press `Ctrl+]` to exit the serial monitor. To change the weather city, run `idf.py menuconfig`, edit the city settings under **RLCD Weather**, then rebuild and flash.

If you have a **complete merged BIN** for this product, use address **0x0**, ESP32-S3, 16 MB Flash, DIO, and 80 MHz. The generated `build/esp32_s3_touch_rlcd_4_2b.bin` is only the application image; do not flash it at 0x0 as a complete firmware. When flashing from source for the first time, use `idf.py flash` so that the speech model and other required contents are included according to `build/flash_args`.

Erasing or writing a full image may clear saved Wi-Fi, volume, and activation information. Set these up again using the steps above. Files on the SD card are unaffected.

Hardware interfaces are shared through the [esp32_s3_touch_rlcd_4_2b BSP](components/esp32_s3_touch_rlcd_4_2b/). For implementation details, see the [Photos](components/Photos/README.md), [Weather](components/Weather/README.md), [Settings](components/Settings/README.md), [Music](components/Music/README.md), and [Mic Test](components/MicTest/README.md) component notes.
