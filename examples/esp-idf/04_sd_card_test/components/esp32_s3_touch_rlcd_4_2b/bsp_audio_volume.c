#include "bsp/esp-bsp.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"

static portMUX_TYPE volume_lock = portMUX_INITIALIZER_UNLOCKED;
static int requested_volume = BSP_AUDIO_VOLUME_DEFAULT;
/* Only the Settings worker writes NVS. An absent/invalid key has no saved value. */
static int saved_volume = -1;

esp_err_t bsp_audio_volume_init(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("rlcd_audio", NVS_READONLY, &handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (ret != ESP_OK) return ret;
    uint8_t value = 0;
    ret = nvs_get_u8(handle, "volume", &value);
    nvs_close(handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (ret != ESP_OK) return ret;
    if (value > 100) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&volume_lock);
    requested_volume = saved_volume = value;
    portEXIT_CRITICAL(&volume_lock);
    return ESP_OK;
}

int bsp_audio_volume_get(void)
{
    portENTER_CRITICAL(&volume_lock);
    int value = requested_volume;
    portEXIT_CRITICAL(&volume_lock);
    return value;
}

esp_err_t bsp_audio_volume_set(int volume)
{
    if (volume < 0 || volume > 100) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&volume_lock);
    requested_volume = volume;
    portEXIT_CRITICAL(&volume_lock);
    return ESP_OK;
}

esp_err_t bsp_audio_volume_save(int volume)
{
    if (volume < 0 || volume > 100) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&volume_lock);
    bool unchanged = saved_volume == volume;
    portEXIT_CRITICAL(&volume_lock);
    if (unchanged) return ESP_OK;
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("rlcd_audio", NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    ret = nvs_set_u8(handle, "volume", (uint8_t)volume);
    if (ret == ESP_OK) ret = nvs_commit(handle);
    nvs_close(handle);
    if (ret == ESP_OK) {
        portENTER_CRITICAL(&volume_lock);
        saved_volume = volume;
        portEXIT_CRITICAL(&volume_lock);
    }
    return ret;
}
