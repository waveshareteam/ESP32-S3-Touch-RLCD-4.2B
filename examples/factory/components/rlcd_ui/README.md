# RLCD App Header

[English](README.md) | [简体中文](README_ZH.md)

Shared 400x48 black header for Weather, Music, 2048, Photos, Sensor, Settings,
Button Test and Mic Test. Drawing/Touch Test, AIChats and Display Test keep
their own layouts.

- Native 32x32 white icon at (12, 8).
- Montserrat 20 title at (51, 13), width 257, ellipsis for long text.
- White back arrow centered in a transparent 65x34 touch target at (323, 7).
  No text caption, border, background or pressed color change.
- App content starts at y=48. Time, volume and track count belong in the body.

Call `rlcd_app_header_create()` while holding the LVGL lock. The returned title
can be updated for child pages. The caller supplies its existing back callback;
the component does not own app state or navigation. All widgets are children
of the supplied parent and are released with it.

The photos viewer fits images into 400x252 below its header; the back arrow returns to
the list, then closes the app. The 2048 board is 236x236 below its header, and
the game result page uses the same header.

Icons are opaque I1 assets generated offline from the existing app glyphs.
No runtime scaling or new background task is used.
