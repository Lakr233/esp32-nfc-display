//
// main.c
// Project TrollNFC
//
// Created by Lakr233 on 2025/05/12.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include "display.h"
#include "nfc.h"

static const char *TAG = "main";

void app_main(void)
{
    srand(time(NULL));

    ESP_LOGI(TAG, "starting application...");

    initialize_display();

    display_text("[+] starting nfc...");
    initialize_nfc();
    display_text("[*] nfc ready");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}