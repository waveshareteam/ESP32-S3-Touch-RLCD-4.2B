#include "photo_service.h"
#include "bsp/esp-bsp.h"
#include "esp_jpeg_dec.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <strings.h>
#include <sys/stat.h>

/* File scanning and data conversion shared by the worker and UI. */
const char *photo_file_name(const char *path)
{
    const char *name = strrchr(path, '/');
    return name ? name + 1 : path;
}

const char *photo_status_message(photo_status_t status)
{
    switch (status) {
    case PHOTO_OK: return "";
    case PHOTO_NO_CARD: return "SD card unavailable. Insert FAT32 card and tap Rescan.";
    case PHOTO_NO_FOLDER: return "Create a photo folder on the SD card, then tap Rescan.";
    case PHOTO_EMPTY: return "No JPG files in /photo. Add photos and tap Rescan.";
    case PHOTO_IO_ERROR: return "Could not read SD card. Return to list and tap Rescan.";
    case PHOTO_BAD_JPEG: return "Invalid or damaged JPEG. Try another photo.";
    case PHOTO_UNSUPPORTED: return "Use baseline JPG (RGB/grayscale), not progressive.";
    case PHOTO_TOO_LARGE: return "Photo too large. Export as 400 x 300 JPG.";
    case PHOTO_NO_MEMORY: return "Not enough memory. Export a smaller JPG.";
    }
    return "Unable to open photo.";
}

void photo_file_list_free(photo_file_list_t *list)
{
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) free(list->paths[i]);
    free(list->paths);
    memset(list, 0, sizeof(*list));
}

static int compare_paths(const void *a, const void *b)
{
    const char *left = *(const char *const *)a, *right = *(const char *const *)b;
    int result = strcasecmp(left, right);
    return result ? result : strcmp(left, right);
}

photo_status_t photo_scan_folder(const char *folder, photo_file_list_t *out)
{
    if (!folder || !out) return PHOTO_IO_ERROR;
    memset(out, 0, sizeof(*out));
    DIR *dir = opendir(folder);
    if (!dir) return errno == ENOENT ? PHOTO_NO_FOLDER : PHOTO_IO_ERROR;
    out->paths = calloc(PHOTO_MAX_FILES, sizeof(char *));
    if (!out->paths) { closedir(dir); return PHOTO_NO_MEMORY; }
    photo_status_t status = PHOTO_OK;
    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) {
            if (errno) status = PHOTO_IO_ERROR;
            break;
        }
        if (entry->d_name[0] == '.') continue;
        const char *extension = strrchr(entry->d_name, '.');
        if (!extension || (strcasecmp(extension, ".jpg") && strcasecmp(extension, ".jpeg"))) continue;
        char path[PHOTO_PATH_MAX];
        int length = snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
        if (length < 0 || (size_t)length >= sizeof(path)) continue;
        struct stat info;
        if (stat(path, &info) != 0) { status = PHOTO_IO_ERROR; break; }
        if (!S_ISREG(info.st_mode)) continue;
        if (out->count == PHOTO_MAX_FILES) { out->limited = true; break; }
        out->paths[out->count] = malloc((size_t)length + 1);
        if (!out->paths[out->count]) { status = PHOTO_NO_MEMORY; break; }
        memcpy(out->paths[out->count++], path, (size_t)length + 1);
    }
    closedir(dir);
    if (status != PHOTO_OK) { photo_file_list_free(out); return status; }
    qsort(out->paths, out->count, sizeof(char *), compare_paths);
    if (!out->count) { photo_file_list_free(out); return PHOTO_EMPTY; }
    return PHOTO_OK;
}

static unsigned be16(const uint8_t *p) { return ((unsigned)p[0] << 8) | p[1]; }

photo_status_t photo_jpeg_info(const uint8_t *bytes, size_t size, uint16_t *width, uint16_t *height)
{
    if (!bytes || !width || !height || size < 4 || bytes[0] != 0xff || bytes[1] != 0xd8)
        return PHOTO_BAD_JPEG;
    if (size > PHOTO_MAX_JPEG_BYTES) return PHOTO_TOO_LARGE;
    bool frame_found = false;
    unsigned w = 0, h = 0;
    size_t pos = 2;
    while (pos < size) {
        if (bytes[pos++] != 0xff) return PHOTO_BAD_JPEG;
        while (pos < size && bytes[pos] == 0xff) ++pos;
        if (pos == size) return PHOTO_BAD_JPEG;
        const unsigned marker = bytes[pos++];
        if (marker == 0 || marker == 0xd8 || marker == 0xd9 || marker == 1 ||
            (marker >= 0xd0 && marker <= 0xd7)) return PHOTO_BAD_JPEG;
        if (size - pos < 2) return PHOTO_BAD_JPEG;
        const unsigned length = be16(bytes + pos);
        if (length < 2 || length > size - pos) return PHOTO_BAD_JPEG;
        /* SOF0 only; other SOF markers include progressive/lossless JPEG. */
        if (marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc) {
            if (marker != 0xc0) return PHOTO_UNSUPPORTED;
            if (frame_found || length < 8) return PHOTO_BAD_JPEG;
            unsigned channels = bytes[pos + 7];
            if (bytes[pos + 2] != 8 || (channels != 1 && channels != 3)) return PHOTO_UNSUPPORTED;
            if (length != 8 + 3 * channels) return PHOTO_BAD_JPEG;
            h = be16(bytes + pos + 3); w = be16(bytes + pos + 5);
            if (!w || !h) return PHOTO_BAD_JPEG;
            if ((size_t)w * h > PHOTO_MAX_PIXELS) return PHOTO_TOO_LARGE;
            frame_found = true;
        }
        if (marker == 0xda) {
            if (!frame_found || length < 6 || size - pos - length < 2) return PHOTO_BAD_JPEG;
            /* Do not accept a truncated file just because the decoder can conceal it. */
            bool end_found = false;
            for (size_t i = pos + length; i + 1 < size; ++i)
                if (bytes[i] == 0xff && bytes[i + 1] == 0xd9) { end_found = true; break; }
            if (!end_found) return PHOTO_BAD_JPEG;
            *width = (uint16_t)w; *height = (uint16_t)h;
            return PHOTO_OK;
        }
        pos += length;
    }
    return PHOTO_BAD_JPEG;
}

static unsigned luminance(const uint8_t *pixel)
{
    unsigned value = pixel[0] | ((unsigned)pixel[1] << 8);
    unsigned r = (value >> 11) * 255 / 31;
    unsigned g = ((value >> 5) & 63) * 255 / 63;
    unsigned b = (value & 31) * 255 / 31;
    return (77 * r + 150 * g + 29 * b + 128) >> 8;
}

bool photo_rgb565_to_i1(const uint8_t *rgb, size_t size, unsigned width, unsigned height,
                        uint8_t *out, size_t capacity)
{
    if (!rgb || !out || !width || !height || width > PHOTO_MAX_PIXELS / height ||
        size < (size_t)width * height * 2 || capacity < PHOTO_I1_BYTES) return false;
    int *errors = calloc(2 * (PHOTO_WIDTH + 2), sizeof(int));
    if (!errors) return false;
    int *current = errors, *next = errors + PHOTO_WIDTH + 2;
    memset(out, 255, PHOTO_I1_BYTES);
    out[0] = out[1] = out[2] = 0; // BGRA palette: opaque black, opaque white.
    unsigned dw = PHOTO_WIDTH, dh = (unsigned)((uint64_t)height * dw / width);
    if (dh > PHOTO_HEIGHT) { dh = PHOTO_HEIGHT; dw = (unsigned)((uint64_t)width * dh / height); }
    if (!dw) dw = 1;
    if (!dh) dh = 1;
    unsigned ox = (PHOTO_WIDTH - dw) / 2, oy = (PHOTO_HEIGHT - dh) / 2;
    for (unsigned y = 0; y < dh; ++y) {
        unsigned y0 = y * height / dh, y1 = (y + 1) * height / dh;
        if (y1 <= y0) y1 = y0 + 1;
        for (unsigned x = 0; x < dw; ++x) {
            unsigned x0 = x * width / dw, x1 = (x + 1) * width / dw;
            if (x1 <= x0) x1 = x0 + 1;
            uint64_t sum = 0;
            for (unsigned sy = y0; sy < y1; ++sy)
                for (unsigned sx = x0; sx < x1; ++sx)
                    sum += luminance(rgb + ((size_t)sy * width + sx) * 2);
            int gray = (int)(sum / ((y1 - y0) * (x1 - x0))) + current[x + 1] / 16;
            if (gray < 0) gray = 0;
            if (gray > 255) gray = 255;
            int quantized = gray >= 128 ? 255 : 0;
            if (!quantized) out[8 + (y + oy) * (PHOTO_WIDTH / 8) + (x + ox) / 8] &= ~(1U << (7 - (x + ox) % 8));
            int error = gray - quantized;
            current[x + 2] += error * 7;
            next[x] += error * 3; next[x + 1] += error * 5; next[x + 2] += error;
        }
        int *swap = current; current = next; next = swap;
        memset(next, 0, (PHOTO_WIDTH + 2) * sizeof(int));
    }
    free(errors);
    return true;
}

/* Background service and hardware access. */
typedef struct {
    uint32_t ticket;
    photo_job_type_t type;
    size_t index;
    char path[PHOTO_PATH_MAX];
} photo_job_t;

static const char *TAG = "photos";
static QueueHandle_t jobs;
static SemaphoreHandle_t result_lock;
static uint32_t latest_ticket;
static bool pending_ready;
static photo_result_t pending;
static bool remount_needed;

static bool job_is_current(uint32_t ticket)
{
    xSemaphoreTake(result_lock, portMAX_DELAY);
    bool current = ticket == latest_ticket;
    xSemaphoreGive(result_lock);
    return current;
}

static photo_status_t scan_card(photo_file_list_t *files)
{
    /* Settings may have mounted the same BSP-owned card before Photos opens. */
    if (remount_needed) {
        if (bsp_sdcard_unmount() != ESP_OK) return PHOTO_NO_CARD;
    }
    remount_needed = false;
    esp_err_t mount_status = bsp_sdcard_mount();
    if (mount_status != ESP_OK) {
        ESP_LOGW(TAG, "SD mount failed: %s", esp_err_to_name(mount_status));
        return PHOTO_NO_CARD;
    }
    photo_status_t status = photo_scan_folder(BSP_SD_MOUNT_POINT "/photo", files);
    if (status == PHOTO_IO_ERROR) remount_needed = true;
    return status;
}

static photo_status_t decode_file(const photo_job_t *job, uint8_t **pixels)
{
    const char *path = job->path;
    const int64_t started = esp_timer_get_time();
    int64_t read_done = started, decode_done = started;
    photo_status_t status = PHOTO_IO_ERROR;
    FILE *file = fopen(path, "rb");
    uint8_t *jpeg = NULL, *rgb = NULL, *mono = NULL;
    jpeg_dec_handle_t decoder = NULL;
    if (!file) return PHOTO_IO_ERROR;
    if (fseek(file, 0, SEEK_END) != 0) goto done;
    long file_size = ftell(file);
    if (file_size <= 0) { status = PHOTO_BAD_JPEG; goto done; }
    if ((unsigned long)file_size > PHOTO_MAX_JPEG_BYTES) { status = PHOTO_TOO_LARGE; goto done; }
    if (fseek(file, 0, SEEK_SET) != 0) goto done;
    jpeg = jpeg_calloc_align(((size_t)file_size + 15) & ~(size_t)15, 16);
    if (!jpeg) { status = PHOTO_NO_MEMORY; goto done; }
    if (fread(jpeg, 1, (size_t)file_size, file) != (size_t)file_size || ferror(file)) goto done;
    fclose(file); file = NULL;
    read_done = esp_timer_get_time();
    if (!job_is_current(job->ticket)) { status = PHOTO_OK; goto done; }

    uint16_t width, height;
    status = photo_jpeg_info(jpeg, (size_t)file_size, &width, &height);
    if (status != PHOTO_OK) goto done;
    jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
    config.output_type = JPEG_PIXEL_FORMAT_RGB565_LE; // Same decoder and pixel order as C5 Photos.
    if (jpeg_dec_open(&config, &decoder) != JPEG_ERR_OK) { status = PHOTO_NO_MEMORY; goto done; }
    jpeg_dec_io_t io = {.inbuf = jpeg, .inbuf_len = (int)file_size};
    jpeg_dec_header_info_t header;
    if (jpeg_dec_parse_header(decoder, &io, &header) != JPEG_ERR_OK ||
        header.width != width || header.height != height) { status = PHOTO_BAD_JPEG; goto done; }
    int output_size = 0;
    if (jpeg_dec_get_outbuf_len(decoder, &output_size) != JPEG_ERR_OK || output_size <= 0 ||
        (size_t)output_size < (size_t)width * height * 2 ||
        (size_t)output_size > PHOTO_MAX_PIXELS * 2 + 65536) { status = PHOTO_TOO_LARGE; goto done; }
    rgb = jpeg_calloc_align(((size_t)output_size + 15) & ~(size_t)15, 16);
    if (!rgb) { status = PHOTO_NO_MEMORY; goto done; }
    io.outbuf = rgb; io.out_size = output_size;
    if (jpeg_dec_process(decoder, &io) != JPEG_ERR_OK) { status = PHOTO_BAD_JPEG; goto done; }
    decode_done = esp_timer_get_time();
    // A foreground request supersedes prefetch at each expensive stage boundary.
    if (!job_is_current(job->ticket)) { status = PHOTO_OK; goto done; }
    /* Release the compressed input before allocating the final display buffer. */
    jpeg_dec_close(decoder); decoder = NULL;
    jpeg_free_align(jpeg); jpeg = NULL;
    mono = malloc(PHOTO_I1_BYTES);
    if (!mono || !photo_rgb565_to_i1(rgb, (size_t)output_size, width, height, mono, PHOTO_I1_BYTES)) {
        status = PHOTO_NO_MEMORY; goto done;
    }
    *pixels = mono; mono = NULL;
    status = PHOTO_OK;
    const int64_t finished = esp_timer_get_time();
    ESP_LOGI(TAG, "%s %s: %ux%u, read=%lu us, jpeg=%lu us, mono=%lu us, total=%lu us",
             job->type == PHOTO_JOB_PREFETCH ? "Prefetch" : "Load", photo_file_name(path), width, height,
             (unsigned long)(read_done - started), (unsigned long)(decode_done - read_done),
             (unsigned long)(finished - decode_done), (unsigned long)(finished - started));
done:
    if (file) fclose(file);
    if (decoder) jpeg_dec_close(decoder);
    if (jpeg) jpeg_free_align(jpeg);
    if (rgb) jpeg_free_align(rgb);
    free(mono);
    return status;
}

static void worker(void *context)
{
    (void)context;
    for (;;) {
        photo_job_t job;
        if (xQueueReceive(jobs, &job, portMAX_DELAY) != pdTRUE) continue;
        if (!job_is_current(job.ticket)) continue;
        photo_result_t result = {.ticket = job.ticket, .job = job.type, .index = job.index};
        if (job.type == PHOTO_JOB_SCAN) result.status = scan_card(&result.files);
        else {
            result.status = decode_file(&job, &result.pixels);
            if (result.status == PHOTO_IO_ERROR) remount_needed = true;
        }
        photo_result_t old = {0};
        xSemaphoreTake(result_lock, portMAX_DELAY);
        bool current = job.ticket == latest_ticket;
        if (current) {
            old = pending;
            pending = result;
            pending_ready = true;
            memset(&result, 0, sizeof(result));
        }
        xSemaphoreGive(result_lock);
        photo_result_clear(&old);
        photo_result_clear(&result); // Also releases work completed after app close / a newer request.
    }
}

bool photo_service_start(void)
{
    if (jobs) return true;
    result_lock = xSemaphoreCreateMutex();
    jobs = xQueueCreate(1, sizeof(photo_job_t));
    if (result_lock && jobs && xTaskCreate(worker, "rlcd_photos", 8192, NULL, 3, NULL) == pdPASS) return true;
    if (jobs) vQueueDelete(jobs);
    if (result_lock) vSemaphoreDelete(result_lock);
    jobs = NULL; result_lock = NULL;
    return false;
}

static bool submit(photo_job_t *job, uint32_t *ticket)
{
    if (!jobs || !ticket) return false;
    photo_result_t old;
    xSemaphoreTake(result_lock, portMAX_DELAY);
    job->ticket = ++latest_ticket;
    *ticket = job->ticket;
    old = pending; memset(&pending, 0, sizeof(pending)); pending_ready = false;
    /* All submissions come from the LVGL thread; last requested photo wins. */
    bool accepted = xQueueOverwrite(jobs, job) == pdTRUE;
    xSemaphoreGive(result_lock);
    photo_result_clear(&old);
    return accepted;
}

bool photo_service_scan(uint32_t *ticket)
{
    photo_job_t job = {.type = PHOTO_JOB_SCAN};
    return submit(&job, ticket);
}

static bool submit_image(const char *path, size_t index, photo_job_type_t type, uint32_t *ticket)
{
    if (!path || strlen(path) >= PHOTO_PATH_MAX) return false;
    photo_job_t job = {.type = type, .index = index};
    snprintf(job.path, sizeof(job.path), "%s", path);
    return submit(&job, ticket);
}

bool photo_service_load(const char *path, size_t index, uint32_t *ticket)
{
    return submit_image(path, index, PHOTO_JOB_LOAD, ticket);
}

bool photo_service_prefetch(const char *path, size_t index, uint32_t *ticket)
{
    return submit_image(path, index, PHOTO_JOB_PREFETCH, ticket);
}

bool photo_service_take_result(photo_result_t *out)
{
    if (!result_lock || !out) return false;
    xSemaphoreTake(result_lock, portMAX_DELAY);
    bool ready = pending_ready;
    if (ready) { *out = pending; memset(&pending, 0, sizeof(pending)); pending_ready = false; }
    xSemaphoreGive(result_lock);
    return ready;
}

void photo_service_cancel(void)
{
    if (!result_lock) return;
    photo_result_t old;
    xSemaphoreTake(result_lock, portMAX_DELAY);
    ++latest_ticket;
    xQueueReset(jobs);
    old = pending; memset(&pending, 0, sizeof(pending)); pending_ready = false;
    xSemaphoreGive(result_lock);
    photo_result_clear(&old);
}
