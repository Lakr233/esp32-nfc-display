//
// nfc.c
// Project TrollNFC
//
// Created by Lakr233 on 2025/05/13.
//

#include "nfc.h"

#include <esp_log.h>
#include <driver/i2c.h>
#include <driver/rc522_spi.h>
#include <picc/rc522_mifare.h>
#include <rc522.h>

#include <display.h>

static const char *TAG = "nfc";

#define RC522_SPI_BUS_GPIO_MISO (19)
#define RC522_SPI_BUS_GPIO_MOSI (23)
#define RC522_SPI_BUS_GPIO_SCLK (18)
#define RC522_SPI_SCANNER_GPIO_SDA (5)

#define RC522_SCANNER_GPIO_RST (-1) // soft-reset
#define RC522_SCANNER_GPIO_IRQ (-1) // not used

#define RC522_SCANNER_SPI_SPEED_HZ (100000) // 100kHz

static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};

static rc522_driver_handle_t driver = NULL;
static rc522_handle_t scanner = NULL;

static void nfc_on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state != RC522_PICC_STATE_ACTIVE)
    {
        ESP_LOGW(TAG, "picc has none activate state: %d", picc->state);
        if (picc->state != RC522_PICC_STATE_IDLE)
        {
            display_text_with_format("[PICC STATE]\n???\n\n%d", picc->state);
        }
        return;
    }

    rc522_picc_print(picc);

    if (!rc522_mifare_type_is_classic_compatible(picc->type))
    {
        ESP_LOGW(TAG, "card is not supported %s", rc522_picc_type_name(picc->type));
        display_text_with_format("UNSUPPORTED\nTYPE\n\n%s", rc522_picc_type_name(picc->type));
        return;
    }

    display_text_with_format(
        "[%s]\n\nCONNECTED %d\nUID: %02X %02X %02X %02X\nSAK: %02X",
        rc522_picc_type_name(picc->type),
        picc->state,
        picc->uid.value[0],
        picc->uid.value[1],
        picc->uid.value[2],
        picc->uid.value[3],
        picc->sak);

    if (rc522_mifare_deauth(scanner, picc) != ESP_OK)
    {
        ESP_LOGW(TAG, "deauth failed");
    }
}

void initialize_nfc(void)
{
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, nfc_on_picc_state_changed, NULL);
    rc522_start(scanner);
}
