#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PHOTO_WIDTH 400
#define PHOTO_HEIGHT 252
#define PHOTO_I1_BYTES (8 + PHOTO_WIDTH * PHOTO_HEIGHT / 8)
#define PHOTO_MAX_FILES 128
#define PHOTO_PATH_MAX 512
#define PHOTO_MAX_JPEG_BYTES (2U * 1024 * 1024)
#define PHOTO_MAX_PIXELS (2U * 1024 * 1024)

typedef enum {
    PHOTO_OK, PHOTO_NO_CARD, PHOTO_NO_FOLDER, PHOTO_EMPTY, PHOTO_IO_ERROR,
    PHOTO_BAD_JPEG, PHOTO_UNSUPPORTED, PHOTO_TOO_LARGE, PHOTO_NO_MEMORY
} photo_status_t;

typedef struct {
    char **paths;
    size_t count;
    bool limited;
} photo_file_list_t;

photo_status_t photo_scan_folder(const char *folder, photo_file_list_t *out);
void photo_file_list_free(photo_file_list_t *list);
const char *photo_file_name(const char *path);
const char *photo_status_message(photo_status_t status);
/* Preflight before decoder allocation: baseline 8-bit, grayscale or three-component JPEG. */
photo_status_t photo_jpeg_info(const uint8_t *bytes, size_t size, uint16_t *width, uint16_t *height);
/* RGB565 little endian -> aspect-fit 400x252 I1 below the app header. */
bool photo_rgb565_to_i1(const uint8_t *rgb, size_t size, unsigned width, unsigned height,
                        uint8_t *out, size_t capacity);

typedef enum { PHOTO_JOB_SCAN, PHOTO_JOB_LOAD, PHOTO_JOB_PREFETCH } photo_job_type_t;
typedef struct {
    uint32_t ticket;
    photo_job_type_t job;
    photo_status_t status;
    size_t index;
    photo_file_list_t files;
    uint8_t *pixels;
} photo_result_t;

bool photo_service_start(void);
bool photo_service_scan(uint32_t *ticket);
bool photo_service_load(const char *path, size_t index, uint32_t *ticket);
bool photo_service_prefetch(const char *path, size_t index, uint32_t *ticket);
bool photo_service_take_result(photo_result_t *out);
void photo_service_cancel(void);
void photo_result_clear(photo_result_t *result);
#ifdef __cplusplus
}
#endif
