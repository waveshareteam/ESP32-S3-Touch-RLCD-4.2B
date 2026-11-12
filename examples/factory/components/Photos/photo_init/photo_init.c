#include "photo_init.h"
#include "photo_service.h"
#include "rlcd_app_header.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_log.h"
#define PHOTO_UI_LOG(...) ESP_LOGI("photos", __VA_ARGS__)
#else
#define PHOTO_UI_LOG(...) ((void)0)
#endif

static photo_ui_t ui;
static lv_timer_t *poll_timer;
static void prefetch_neighbors(void);

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int w, const lv_font_t *font)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_pos(obj, x, y); lv_obj_set_width(obj, w);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_label_set_text(obj, text);
    return obj;
}

static lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int w, lv_event_cb_t event)
{
    lv_obj_t *obj = lv_button_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, 34);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, lv_color_black(), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 6, 0);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_obj_set_style_bg_color(obj, lv_color_black(), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_STATE_PRESSED);
    lv_obj_t *text_obj = lv_label_create(obj);
    lv_label_set_text(text_obj, text); lv_obj_center(text_obj);
    if (event) lv_obj_add_event_cb(obj, event, LV_EVENT_CLICKED, NULL);
    return obj;
}

static void clear_frame(void)
{
    lv_image_cache_drop(&ui.frame);
    /* Keep the image descriptor alive until the child image itself is deleted. */
    ui.pixels = NULL; ui.frame.data = NULL;
}

static photo_cache_entry_t *cache_find(size_t index)
{
    for (size_t i = 0; i < PHOTO_CACHE_SLOTS; ++i)
        if (ui.cache[i].valid && ui.cache[i].index == index) return &ui.cache[i];
    return NULL;
}

static bool in_window(size_t index)
{
    return ui.files.count && (index == ui.selected ||
           index == (ui.selected + 1) % ui.files.count ||
           index == (ui.selected + ui.files.count - 1) % ui.files.count);
}

static void cache_release(photo_cache_entry_t *entry)
{
    free(entry->pixels);
    memset(entry, 0, sizeof(*entry));
}

static void cache_clear(void)
{
    clear_frame();
    for (size_t i = 0; i < PHOTO_CACHE_SLOTS; ++i) cache_release(&ui.cache[i]);
}

static void cache_prune(void)
{
    for (size_t i = 0; i < PHOTO_CACHE_SLOTS; ++i) {
        photo_cache_entry_t *entry = &ui.cache[i];
        // Keep the old visible photo alive until its replacement is ready.
        if (entry->valid && !in_window(entry->index) && (!entry->pixels || entry->pixels != ui.pixels))
            cache_release(entry);
    }
}

static photo_cache_entry_t *cache_store(photo_result_t *result)
{
    cache_prune();
    photo_cache_entry_t *entry = cache_find(result->index);
    if (!entry) {
        for (size_t i = 0; i < PHOTO_CACHE_SLOTS; ++i)
            if (!ui.cache[i].valid) { entry = &ui.cache[i]; break; }
    }
    if (!entry) {
        for (size_t i = 0; i < PHOTO_CACHE_SLOTS; ++i)
            if (!ui.cache[i].pixels || ui.cache[i].pixels != ui.pixels) { entry = &ui.cache[i]; break; }
    }
    if (!entry) return NULL;
    if (entry->pixels && entry->pixels == ui.pixels) clear_frame();
    cache_release(entry);
    *entry = (photo_cache_entry_t){.valid = true, .index = result->index,
                                 .status = result->status, .pixels = result->pixels};
    result->pixels = NULL;
    return entry;
}

static void show_message(const char *text)
{
    lv_obj_add_flag(ui.image, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui.message, text);
    lv_obj_remove_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
}

static void display_name(char *out, size_t capacity, const char *path)
{
    const unsigned char *name = (const unsigned char *)photo_file_name(path);
    size_t i = 0;
    for (; name[i] && i + 1 < capacity; ++i) out[i] = name[i] >= 32 && name[i] < 127 ? (char)name[i] : '?';
    out[i] = 0;
}

static void show_cached(photo_cache_entry_t *entry, bool cache_hit)
{
    if (entry->status != PHOTO_OK || !entry->pixels) {
        clear_frame();
        show_message(photo_status_message(entry->status == PHOTO_OK ? PHOTO_BAD_JPEG : entry->status));
        cache_prune();
        return;
    }
    clear_frame();
    ui.pixels = entry->pixels;
    ui.frame = (lv_image_dsc_t){
        .header.magic = LV_IMAGE_HEADER_MAGIC, .header.cf = LV_COLOR_FORMAT_I1,
        .header.w = PHOTO_WIDTH, .header.h = PHOTO_HEIGHT, .header.stride = PHOTO_WIDTH / 8,
        .data_size = PHOTO_I1_BYTES, .data = ui.pixels
    };
    lv_image_set_src(ui.image, &ui.frame);
    lv_obj_invalidate(ui.image);
    lv_obj_remove_flag(ui.image, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
    PHOTO_UI_LOG("UI ready %s: cache=%s, wait=%lu ms (before display flush)",
                 photo_file_name(ui.files.paths[entry->index]), cache_hit ? "hit" : "miss",
                 (unsigned long)lv_tick_elaps(ui.requested_at));
    (void)cache_hit;
    cache_prune();
}

static void prefetch_neighbors(void)
{
    if (ui.paused || ui.load_pending || !ui.files.count || lv_obj_has_flag(ui.viewer, LV_OBJ_FLAG_HIDDEN)) return;
    size_t next = (ui.selected + 1) % ui.files.count;
    size_t previous = (ui.selected + ui.files.count - 1) % ui.files.count;
    const size_t neighbors[] = {ui.prefer_previous ? previous : next, ui.prefer_previous ? next : previous};
    for (size_t i = 0; i < 2; ++i) {
        size_t index = neighbors[i];
        if (index == ui.selected || cache_find(index)) continue;
        if (photo_service_prefetch(ui.files.paths[index], index, &ui.ticket)) {
            ui.loading_index = index;
            ui.load_pending = true;
        }
        return;
    }
}

static void request_photo(size_t index)
{
    if (!ui.files.count || index >= ui.files.count) return;
    ui.selected = index;
    ui.requested_at = lv_tick_get();
    lv_obj_remove_flag(ui.viewer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui.viewer);
    photo_cache_entry_t *entry = cache_find(index);
    if (entry) {
        if (ui.load_pending && !in_window(ui.loading_index)) {
            photo_service_cancel();
            ui.load_pending = false;
            ui.ticket = 0;
        }
        show_cached(entry, true);
        prefetch_neighbors();
        return;
    }
    cache_prune();
    if (ui.pixels) {
        lv_obj_remove_flag(ui.image, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
    } else show_message("Loading photo...");
    // Reuse a prefetch already decoding this exact photo instead of restarting it.
    if (ui.load_pending && ui.loading_index == index) return;
    ui.load_pending = photo_service_load(ui.files.paths[index], index, &ui.ticket);
    ui.loading_index = index;
    if (!ui.load_pending)
        show_message("Unable to start decoder. Close and reopen Photos.");
}

static void select_event(lv_event_t *event) { request_photo((size_t)(uintptr_t)lv_event_get_user_data(event)); }
static void previous_photo(void)
{
    ui.prefer_previous = true;
    if (ui.files.count) request_photo((ui.selected + ui.files.count - 1) % ui.files.count);
}
static void next_photo(void)
{
    ui.prefer_previous = false;
    if (ui.files.count) request_photo((ui.selected + 1) % ui.files.count);
}

bool photo_ui_back_to_list(void)
{
    if (!ui.viewer || lv_obj_has_flag(ui.viewer, LV_OBJ_FLAG_HIDDEN)) return false;
    lv_obj_add_flag(ui.viewer, LV_OBJ_FLAG_HIDDEN);
    photo_service_cancel();
    ui.ticket = 0;
    ui.load_pending = false;
    clear_frame();
    return true;
}
static void back_event(lv_event_t *event)
{
    (void)event;
    if (ui.on_back) ui.on_back(ui.context);
}

static void scan(void)
{
    photo_service_cancel();
    photo_ui_back_to_list();
    cache_clear();
    ui.load_pending = false;
    lv_obj_clean(ui.list);
    photo_file_list_free(&ui.files);
    lv_label_set_text(ui.status, "Reading SD card /photo...");
    lv_obj_add_state(ui.rescan, LV_STATE_DISABLED);
    if (!photo_service_scan(&ui.ticket)) {
        lv_label_set_text(ui.status, "Unable to start SD service. Close and reopen Photos.");
        lv_obj_remove_state(ui.rescan, LV_STATE_DISABLED);
    }
}
static void rescan_event(lv_event_t *event) { (void)event; scan(); }

static void viewer_event(lv_event_t *event)
{
    if (lv_event_get_target(event) != ui.viewer) return;
    lv_indev_t *touch = lv_indev_active();
    if (!touch) return;
    lv_dir_t direction = lv_indev_get_gesture_dir(touch);
    lv_indev_wait_release(touch);
    if (direction == LV_DIR_LEFT) next_photo();
    else if (direction == LV_DIR_RIGHT) previous_photo();
    else if (direction == LV_DIR_TOP) photo_ui_back_to_list();
}

static void populate_list(void)
{
    lv_label_set_text_fmt(ui.status, "%u JPG photos in /photo%s",
                         (unsigned)ui.files.count, ui.files.limited ? " (first 128)" : "");
    for (size_t i = 0; i < ui.files.count; ++i) {
        lv_obj_t *row = button(ui.list, "", 0, 0, 360, NULL);
        lv_obj_set_width(row, LV_PCT(100)); lv_obj_set_height(row, 42);
        lv_obj_t *text = lv_obj_get_child(row, 0);
        char name[256]; display_name(name, sizeof(name), ui.files.paths[i]);
        lv_label_set_text(text, name);
        lv_obj_set_width(text, LV_PCT(92));
        lv_label_set_long_mode(text, LV_LABEL_LONG_DOT);
        lv_obj_add_event_cb(row, select_event, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
}

static void poll(lv_timer_t *timer)
{
    (void)timer;
    photo_result_t result = {0};
    if (!ui.root || !photo_service_take_result(&result)) return;
    if (result.ticket == ui.ticket) {
        if (result.job == PHOTO_JOB_SCAN) {
            lv_obj_remove_state(ui.rescan, LV_STATE_DISABLED);
            if (result.status == PHOTO_OK) {
                ui.files = result.files; memset(&result.files, 0, sizeof(result.files));
                populate_list();
            } else lv_label_set_text(ui.status, photo_status_message(result.status));
        } else if (ui.load_pending && result.index == ui.loading_index) {
            ui.load_pending = false;
            if (in_window(result.index)) {
                photo_cache_entry_t *entry = cache_store(&result);
                if (entry && result.index == ui.selected) show_cached(entry, false);
            }
            prefetch_neighbors();
        }
    }
    photo_result_clear(&result);
}

static void deleted(lv_event_t *event)
{
    if (lv_event_get_target(event) != ui.root) return;
    if (poll_timer) { lv_timer_delete(poll_timer); poll_timer = NULL; }
    photo_service_cancel();
    cache_clear();
    photo_file_list_free(&ui.files);
    memset(&ui, 0, sizeof(ui));
}

bool init_photo_ui_screen(lv_obj_t *parent, int32_t x, int32_t y, photo_back_cb_t back, void *context)
{
    if (!parent || !photo_service_start()) return false;
    deinit_photo_ui();
    ui.on_back = back; ui.context = context;
    ui.root = box(parent, x, y, PHOTO_WIDTH, 300);
    lv_obj_set_user_data(ui.root, &ui);
    lv_obj_add_event_cb(ui.root, deleted, LV_EVENT_DELETE, NULL);
    rlcd_app_header_t header = rlcd_app_header_create(ui.root, "Photos", &rlcd_header_photos, back_event, NULL);
    ui.back = header.back;
    ui.rescan = button(ui.root, "Rescan", 300, 56, 88, rescan_event);
    ui.status = label(ui.root, "", 16, 56, 272, &lv_font_montserrat_12);
    lv_label_set_long_mode(ui.status, LV_LABEL_LONG_WRAP);
    ui.list = box(ui.root, 12, 96, 376, 192);
    lv_obj_add_flag(ui.list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui.list, LV_DIR_VER);
    lv_obj_set_style_pad_all(ui.list, 4, 0);
    lv_obj_set_style_pad_row(ui.list, 4, 0);
    lv_obj_set_flex_flow(ui.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(ui.list, lv_color_black(), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(ui.list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    ui.viewer = box(ui.root, 0, 0, PHOTO_WIDTH, 300);
    lv_obj_remove_flag(ui.viewer, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(ui.viewer, viewer_event, LV_EVENT_GESTURE, NULL);
    lv_obj_add_flag(ui.viewer, LV_OBJ_FLAG_HIDDEN);
    ui.image = lv_image_create(ui.viewer);
    lv_obj_set_pos(ui.image, 0, RLCD_APP_HEADER_HEIGHT);
    lv_obj_remove_flag(ui.image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui.image, LV_OBJ_FLAG_HIDDEN);
    ui.message = label(ui.viewer, "", 28, 160, 344, &lv_font_montserrat_16);
    lv_label_set_long_mode(ui.message, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui.message, LV_TEXT_ALIGN_CENTER, 0);
    rlcd_app_header_create(ui.viewer, "Photos", &rlcd_header_photos, back_event, NULL);
    poll_timer = lv_timer_create(poll, 20, NULL);
    if (!poll_timer) { deinit_photo_ui(); return false; }
    scan();
    return true;
}

void deinit_photo_ui(void) { if (ui.root) lv_obj_delete(ui.root); }
void pause_photo_ui(bool paused)
{
    if (!poll_timer) return;
    ui.paused = paused;
    if (paused) lv_timer_pause(poll_timer);
    else { poll(NULL); prefetch_neighbors(); lv_timer_resume(poll_timer); }
}
