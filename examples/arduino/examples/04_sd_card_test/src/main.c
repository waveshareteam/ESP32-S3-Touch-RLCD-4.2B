/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "example_support.h"
#include "esp_log.h"

static const char *TAG = "sd_card";
static const char test_text[] = "ESP32-S3-Touch-RLCD-4.2B SD test\n";
static const char test_path[] = BSP_SD_MOUNT_POINT "/test.txt";

typedef struct {
    esp_err_t error;
    const char *status;
    const char *hint;
    char contents[sizeof(test_text)];
} sd_test_result_t;

static void run_sd_test(sd_test_result_t *result)
{
    result->status = "SD card not mounted";
    result->hint = "Insert a FAT32 microSD card,\nthen press RESET to retry.";
    result->error = bsp_sdcard_acquire();
    if (result->error != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed: %s. Insert a FAT32 microSD card and press RESET.",
                 esp_err_to_name(result->error));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SD_MOUNT_POINT);

    result->error = ESP_FAIL;
    result->status = "File write failed";
    result->hint = "Check the card is writable,\nthen press RESET to retry.";
    FILE *file = fopen(test_path, "wb");
    if (!file) {
        ESP_LOGE(TAG, "Open for writing failed: %s", strerror(errno));
        goto cleanup;
    }
    size_t bytes = fwrite(test_text, 1, sizeof(test_text) - 1, file);
    int closed = fclose(file);
    if (bytes != sizeof(test_text) - 1 || closed != 0) {
        ESP_LOGE(TAG, "Write or close failed: %s", test_path);
        goto cleanup;
    }
    ESP_LOGI(TAG, "Write successful: %s (%u bytes)", test_path, (unsigned)bytes);

    result->status = "File read failed";
    result->hint = "Check the card, then press\nRESET to repeat the test.";
    file = fopen(test_path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Open for reading failed: %s", strerror(errno));
        goto cleanup;
    }
    bytes = fread(result->contents, 1, sizeof(result->contents) - 1, file);
    int extra = fgetc(file);
    bool read_error = ferror(file);
    closed = fclose(file);
    result->contents[bytes] = '\0';
    if (read_error || closed != 0 || extra != EOF || bytes != sizeof(test_text) - 1 ||
        memcmp(test_text, result->contents, bytes) != 0) {
        result->contents[0] = '\0';
        ESP_LOGE(TAG, "Read failed or file content did not match: %s", test_path);
        goto cleanup;
    }
    ESP_LOGI(TAG, "Read successful: %s", result->contents);
    result->error = ESP_OK;
    result->status = "Write / read passed";
    result->hint = "Test complete.\nPress RESET to run again.";

cleanup:
    ESP_LOGI(TAG, "Write/read verification: %s", esp_err_to_name(result->error));
    bsp_sdcard_release();
    esp_err_t unmount = bsp_sdcard_unmount();
    if (unmount == ESP_OK) {
        ESP_LOGI(TAG, "SD card unmounted; test.txt remains on the card");
    } else {
        ESP_LOGW(TAG, "Unmount failed: %s", esp_err_to_name(unmount));
        if (result->error == ESP_OK) {
            result->error = unmount;
            result->status = "Card cleanup failed";
            result->hint = "Read/write passed. Check\nthe serial log, then RESET.";
        }
    }
}

static void show_result(lv_obj_t *screen, const sd_test_result_t *result)
{
    lv_obj_t *status = example_label(screen, result->status, 20, 65, 360, &lv_font_montserrat_24);
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *card = example_box(screen, 20, 105, 360, 100);
    example_label(card, test_path, 14, 12, 332, &lv_font_montserrat_16);
    example_label(card, result->contents[0] ? result->contents : "No verified data.",
                  14, 43, 332, &lv_font_montserrat_16);
    lv_obj_t *hint = example_label(screen, result->hint, 20, 222, 360, &lv_font_montserrat_16);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
}

void example_main(void)
{
    lv_obj_t *screen = example_screen_begin("SD Card Test");
    lv_obj_t *waiting = example_label(screen, "Checking microSD card...", 20, 134, 360, &lv_font_montserrat_20);
    lv_obj_set_style_text_align(waiting, LV_TEXT_ALIGN_CENTER, 0);
    bsp_display_unlock();
    sd_test_result_t result = {0};
    run_sd_test(&result);
    ESP_ERROR_CHECK(bsp_display_lock(UINT32_MAX) ? ESP_OK : ESP_ERR_TIMEOUT);
    lv_obj_delete(waiting);
    show_result(screen, &result);
    bsp_display_unlock();
}
