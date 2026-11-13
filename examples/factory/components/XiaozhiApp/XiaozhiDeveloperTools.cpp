/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XiaozhiDeveloperTools.hpp"

#include "bsp/esp-bsp.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_mcp_engine.h"
#include "esp_mcp_property.h"
#include "esp_mcp_tool.h"

namespace esp_brookesia::apps {

namespace {

constexpr char TAG[] = "XiaozhiTools";

esp_err_t writeVolumeStatus(esp_mcp_tool_result_t *result)
{
    cJSON *status = cJSON_CreateObject();
    if (!status) {
        return ESP_ERR_NO_MEM;
    }
    cJSON *speaker = cJSON_AddObjectToObject(status, "audio_speaker");
    if (!speaker || !cJSON_AddNumberToObject(speaker, "volume", bsp_audio_volume_get())) {
        cJSON_Delete(status);
        return ESP_ERR_NO_MEM;
    }
    char *json = cJSON_PrintUnformatted(status);
    cJSON_Delete(status);
    if (!json) {
        return ESP_ERR_NO_MEM;
    }
    const esp_err_t ret = esp_mcp_tool_result_add_text(result, json);
    cJSON_free(json);
    return ret;
}

esp_err_t getDeviceStatus(
    const esp_mcp_property_list_t *properties,
    esp_mcp_tool_result_t *result
)
{
    (void)properties;
    return writeVolumeStatus(result);
}

esp_err_t setSpeakerVolume(
    const esp_mcp_property_list_t *properties,
    esp_mcp_tool_result_t *result
)
{
    if (!properties) {
        (void)esp_mcp_tool_result_add_text(result, "Missing volume argument (0-100).");
        return ESP_ERR_INVALID_ARG;
    }
    const int volume = esp_mcp_property_list_get_property_int(properties, "volume");
    if (volume < 0 || volume > 100) {
        (void)esp_mcp_tool_result_add_text(result, "Volume must be between 0 and 100.");
        return ESP_ERR_INVALID_ARG;
    }

    // Only update the shared request; the playback task applies codec changes.
    const esp_err_t ret = bsp_audio_volume_set(volume);
    if (ret != ESP_OK) {
        (void)esp_mcp_tool_result_add_text(result, "Failed to set speaker volume.");
        return ret;
    }
    ESP_LOGI(TAG, "Speaker volume set to %d%% by MCP", volume);
    return writeVolumeStatus(result);
}

} // namespace

esp_err_t registerXiaozhiDeveloperTools(esp_mcp_t *mcp)
{
    if (!mcp) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_mcp_tool_t *status_tool = esp_mcp_tool_create_ex(
        "self.get_device_status", nullptr,
        "Get the current speaker volume (0-100). Call this before relative "
        "volume changes such as louder or quieter.",
        getDeviceStatus
    );
    if (!status_tool) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t ret = esp_mcp_add_tool(mcp, status_tool);
    if (ret != ESP_OK) {
        esp_mcp_tool_destroy(status_tool);
        return ret;
    }

    esp_mcp_tool_t *volume_tool = esp_mcp_tool_create_ex(
        "self.audio_speaker.set_volume", nullptr,
        "Set the speaker volume to an absolute integer percentage from 0 to 100. "
        "0 mutes the speaker. For louder or quieter, get the current device "
        "status first, then set the new volume. This changes the current "
        "runtime volume shared with Settings.",
        setSpeakerVolume
    );
    if (!volume_tool) {
        return ESP_ERR_NO_MEM;
    }
    esp_mcp_property_t *volume_property =
        esp_mcp_property_create_with_range("volume", 0, 100);
    if (!volume_property) {
        esp_mcp_tool_destroy(volume_tool);
        return ESP_ERR_NO_MEM;
    }
    ret = esp_mcp_tool_add_property(volume_tool, volume_property);
    if (ret != ESP_OK) {
        esp_mcp_property_destroy(volume_property);
        esp_mcp_tool_destroy(volume_tool);
        return ret;
    }
    ret = esp_mcp_add_tool(mcp, volume_tool);
    if (ret != ESP_OK) {
        esp_mcp_tool_destroy(volume_tool);
        return ret;
    }

    ESP_LOGI(TAG, "Registered MCP device status and speaker volume tools (0-100)");
    return ESP_OK;
}

} // namespace esp_brookesia::apps
