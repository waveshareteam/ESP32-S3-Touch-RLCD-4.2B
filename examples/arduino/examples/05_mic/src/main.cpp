/* SPDX-License-Identifier: Apache-2.0 */
#include "MicTest.hpp"

extern "C" void example_main(void)
{
    // NVS access stays on the internal-RAM main stack, before audio starts.
    example_nvs_init();
    ESP_ERROR_CHECK(bsp_audio_volume_init());
    ESP_ERROR_CHECK(bsp_audio_volume_set(100));
    example_screen_begin(nullptr);
    static MicTest app;
    bool started = app.run();
    bsp_display_unlock();
    ESP_ERROR_CHECK(started ? ESP_OK : ESP_ERR_NO_MEM);
}
