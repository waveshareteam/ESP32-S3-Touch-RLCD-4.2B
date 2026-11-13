/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include <Arduino.h>
#include <board_config.h>

static void run_example(void *context)
{
    (void)context;
    example_main();
    vTaskDelete(nullptr);
}

void setup()
{
    Serial.begin(115200);
    // Flash and peripheral operations use an internal-RAM worker stack.
    BaseType_t started = xTaskCreate(run_example, "example", 12288, nullptr, 2, nullptr);
    ESP_ERROR_CHECK(started == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

void loop()
{
    delay(1000);
}
