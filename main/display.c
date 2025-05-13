//
// display.c
// Project TrollNFC
//
// Created by Lakr233 on 2025/05/12.
//

#include "display.h"

#include <font8x8/font8x8.h>

#include <esp_log.h>
#include <driver/i2c.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_ssd1306.h>

static const char *TAG = "display";

#define I2C_HOST 0
#define I2C_SDA_PIN 13
#define I2C_SCL_PIN 14
#define I2C_FREQ_HZ 400000 // 400kHz

#define OLED_ADDR 0x3C // SSD1306 I2C address, usually 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define DRAW_FLIP_X 1
#define DRAW_FLIP_Y 1

static esp_lcd_panel_handle_t panel_handle = NULL;

void initialize_display(void)
{
    ESP_LOGI(TAG, "initializing I2C bus");

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ};
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, i2c_conf.mode, 0, 0, 0));

    ESP_LOGI(TAG, "initializing SSD1306 panel");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_ADDR,
        .control_phase_bytes = 1, // command consists of one control byte
        .dc_bit_offset = 6,       // DC bit (data/command selection bit) position in the control byte
        .lcd_cmd_bits = 8,        // LCD command width, in bits
        .lcd_param_bits = 8,      // LCD parameter width, in bits
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_HEIGHT};
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,  // 1 bit per pixel (monochrome)
        .reset_gpio_num = -1, // do not use reset pin
        .vendor_config = &ssd1306_config};

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

void clear_display(void)
{
    if (panel_handle == NULL)
    {
        ESP_LOGE(TAG, "panel not initialized");
        return;
    }

    uint8_t *buffer = malloc(OLED_WIDTH * OLED_HEIGHT / 8);
    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "failed to allocate display buffer");
        return;
    }
    memset(buffer, 0, OLED_WIDTH * OLED_HEIGHT / 8);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, OLED_WIDTH, OLED_HEIGHT, buffer);
    free(buffer);
}

void display_text(const char *text)
{
    if (panel_handle == NULL)
    {
        ESP_LOGE(TAG, "Panel not initialized");
        return;
    }

    clear_display();

    int text_len = strlen(text);
    int buffer_width = OLED_WIDTH;
    int buffer_height = OLED_HEIGHT;

    uint8_t **screen = (uint8_t **)malloc(buffer_height * sizeof(uint8_t *));
    if (screen == NULL)
    {
        ESP_LOGE(TAG, "failed to allocate screen buffer");
        return;
    }

    for (int y = 0; y < buffer_height; y++)
    {
        screen[y] = (uint8_t *)malloc(buffer_width * sizeof(uint8_t));
        if (screen[y] == NULL)
        {
            for (int j = 0; j < y; j++)
            {
                free(screen[j]);
            }
            free(screen);
            ESP_LOGE(TAG, "failed to allocate screen row buffer");
            return;
        }
        memset(screen[y], 0, buffer_width * sizeof(uint8_t));
    }

    int x_pos = 0;
    int y_pos = 0;

    const int char_width = 8;
    const int char_height = 8;

    const int chars_per_line = buffer_width / char_width;

    for (int i = 0; i < text_len; i++)
    {
        if (x_pos >= chars_per_line * char_width)
        {
            x_pos = 0;
            y_pos += char_height;

            if (y_pos >= buffer_height)
            {
                break;
            }
        }

        char character = text[i];
        if (character == '\n')
        {
            if (x_pos == 0)
            {

                char prev = i > 0 ? text[i - 1] : 0;
                if (prev != '\n') continue;
            }

            x_pos = 0;
            y_pos += char_height;
            continue;
        }

        char *char_bitmap = font8x8_basic[(unsigned char)text[i]];

        for (int y = 0; y < char_height; y++)
        {
            uint8_t line = char_bitmap[y];

            for (int x = 0; x < char_width; x++)
            {
                if (line & (1 << x))
                {
                    int screen_x = x_pos + x;
                    int screen_y = y_pos + y;

                    if (screen_x < buffer_width && screen_y < buffer_height)
                    {
                        screen[screen_y][screen_x] = 1;
                    }
                }
            }
        }

        x_pos += char_width;
    }

    uint8_t *buffer = malloc(buffer_width * buffer_height / 8);
    if (buffer == NULL)
    {
        for (int y = 0; y < buffer_height; y++)
        {
            free(screen[y]);
        }
        free(screen);
        ESP_LOGE(TAG, "failed to allocate display buffer");
        return;
    }
    memset(buffer, 0, buffer_width * buffer_height / 8);

    for (int y = 0; y < buffer_height; y++)
    {
        for (int x = 0; x < buffer_width; x++)
        {
            if (screen[y][x])
            {
                int byte_index, bit_index;

                int display_x = DRAW_FLIP_X ? (buffer_width - 1 - x) : x;
                int display_y = DRAW_FLIP_Y ? (buffer_height - 1 - y) : y;

                byte_index = (display_y / 8) * buffer_width + display_x;
                bit_index = display_y % 8;

                buffer[byte_index] |= (1 << bit_index);
            }
        }
    }

    for (int y = 0; y < buffer_height; y++)
    {
        free(screen[y]);
    }
    free(screen);

    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, buffer_width, buffer_height, buffer);
    free(buffer);
}

void display_text_with_format(const char *text, ...)
{
    va_list args;
    va_start(args, text);

    char formatted_text[256];
    vsnprintf(formatted_text, sizeof(formatted_text), text, args);

    display_text(formatted_text);

    va_end(args);
}