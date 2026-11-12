#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#define MUSIC_PATH_MAX 320
#define MUSIC_TITLE_MAX 128
typedef enum { MUSIC_OK, MUSIC_NO_FOLDER, MUSIC_EMPTY, MUSIC_IO_ERROR,
               MUSIC_NO_MEMORY, MUSIC_BAD_MP3, MUSIC_CANCELLED } music_result_t;
typedef struct { char **paths; size_t count; } music_files_t;
typedef struct {
    uint32_t duration_ms, sample_rate;
    unsigned channels;
    char title[MUSIC_TITLE_MAX], artist[96];
} music_info_t;
typedef bool (*music_cancel_fn)(void *context);

music_result_t music_scan(const char *folder, music_files_t *files, music_cancel_fn cancel, void *ctx);
void music_files_free(music_files_t *files);
/* Parse MPEG Layer III frames without decoding; Xing/Info and VBRI fast paths.
 * Files without a frame count are scanned, so VBR duration is not a CBR estimate. */
music_result_t music_probe(const char *path, music_info_t *info, music_cancel_fn cancel, void *ctx);
void music_title(const char *path, char *title, size_t capacity);
void music_format_time(uint32_t ms, char *out, size_t capacity);

typedef enum { MUSIC_UI_LOADING, MUSIC_UI_READY, MUSIC_UI_NO_CARD, MUSIC_UI_NO_FOLDER,
               MUSIC_UI_EMPTY, MUSIC_UI_ERROR } music_ui_status_t;
typedef struct {
    music_ui_status_t status;
    size_t index, count;
    uint32_t position_ms, duration_ms;
    bool playing;
    char title[MUSIC_TITLE_MAX];
    char detail[96];
} music_snapshot_t;
/* UI calls are nonblocking. The worker owns decoder/FILE/codec lifetimes. */
bool music_service_open(void);
void music_service_close(void);
/* Leaving the app releases I2S for voice. Returning retains the selection,
 * stopped at the beginning; the user explicitly presses Play again. */
void music_service_suspend(bool suspended);
void music_service_toggle(void);
void music_service_step(int direction);
void music_service_snapshot(music_snapshot_t *out);
#ifdef __cplusplus
}
#endif
