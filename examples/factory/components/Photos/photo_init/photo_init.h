#pragma once
#include "lvgl.h"
#include "photo_service.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*photo_back_cb_t)(void *context);
#define PHOTO_CACHE_SLOTS 3
typedef struct {
    bool valid;
    size_t index;
    photo_status_t status;
    uint8_t *pixels;
} photo_cache_entry_t;
typedef struct {
    lv_obj_t *root, *list, *status, *back, *rescan;
    lv_obj_t *viewer, *image, *message;
    lv_image_dsc_t frame;
    uint8_t *pixels; // Borrowed from cache; only the LVGL thread owns these buffers.
    photo_cache_entry_t cache[PHOTO_CACHE_SLOTS];
    photo_file_list_t files;
    uint32_t ticket;
    size_t selected;
    size_t loading_index;
    uint32_t requested_at;
    bool load_pending, paused, prefer_previous;
    photo_back_cb_t on_back;
    void *context;
} photo_ui_t;

bool init_photo_ui_screen(lv_obj_t *parent, int32_t x, int32_t y, photo_back_cb_t back, void *context);
void deinit_photo_ui(void);
void pause_photo_ui(bool paused);
bool photo_ui_back_to_list(void);
#ifdef __cplusplus
}
#endif
