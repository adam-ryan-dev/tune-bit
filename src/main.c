#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "font/lv_font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stdio.h>

#define TAG "Tune Bit"

#define PIN_LCD_MOSI 9
#define PIN_LCD_SCLK 7
#define PIN_LCD_CS 2
#define PIN_LCD_DC 4
#define PIN_LCD_RST 1
#define PIN_LCD_BL 44

#define LCD_SPI_HOST SPI2_HOST

#define LCD_H_RES 240
#define LCD_V_RES 280
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)

esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t panel_io;
lv_display_t *display;

static void lcd_init(void) {
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &panel_io));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, 0, 20));

    // Depending on your module:
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
}

static void lvgl_init(void) {
    const lvgl_port_cfg_t cfg = ESP_LVGL_PORT_INIT_CONFIG();

    ESP_ERROR_CHECK(lvgl_port_init(&cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io,
        .panel_handle = panel,
        .buffer_size = LCD_H_RES * 40,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,

        .rotation =
            {
                .swap_xy = false,
                .mirror_x = false,
                .mirror_y = false,
            },

        .flags =
            {
                .buff_dma = true,
                .buff_spiram = false,
                .sw_rotate = false,
                .swap_bytes = true,
            },
    };

    display = lvgl_port_add_disp(&disp_cfg);

    assert(display != NULL);
}

static lv_obj_t *label;
static void create_ui(void) {
    lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);

    label = lv_label_create(screen);

    lv_label_set_text(label, "Hello Tune Bit!");

    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);

    lv_obj_center(label);
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Initializing st7789v2 display with lvgl");
    gpio_config_t bl_config = {
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
        .mode = GPIO_MODE_OUTPUT,
    };

    ESP_ERROR_CHECK(gpio_config(&bl_config));
    gpio_set_level(PIN_LCD_BL, 1);

    lcd_init();

    lvgl_init();

    if (lvgl_port_lock(0)) {
        ESP_LOGI(TAG, "Initilaized st7789v2 display");
        create_ui();
        lvgl_port_unlock();
    }
}
