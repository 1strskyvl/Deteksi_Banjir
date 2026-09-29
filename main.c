#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

//  PIN ASSIGNMENT
#define TRIG_PIN         GPIO_NUM_5
#define ECHO_PIN         GPIO_NUM_18

#define BUZZER_PIN       GPIO_NUM_26
#define LED_MERAH        GPIO_NUM_27
#define LED_KUNING       GPIO_NUM_14
#define LED_HIJAU        GPIO_NUM_12

// KONFIGURASI I2C LCD
#define I2C_MASTER_NUM   I2C_NUM_0
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_FREQ_HZ 100000
#define LCD_ADDR         0x27

#define LCD_BACKLIGHT    0x08
#define ENABLE           0x04
#define REGISTER_SELECT  0x01

//  DRIVER LCD I2C LOW-LEVEL 
esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void lcd_write_nibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;
    
    uint8_t data_en = data | ENABLE;
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_en, 1, pdMS_TO_TICKS(50));
    ets_delay_us(1);
    
    uint8_t data_dis = data & ~ENABLE;
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data_dis, 1, pdMS_TO_TICKS(50));
    ets_delay_us(50);
}

void lcd_send_cmd(uint8_t cmd) {
    lcd_write_nibble(cmd & 0xF0, 0);
    lcd_write_nibble((cmd << 4) & 0xF0, 0);
}

void lcd_send_data(uint8_t data) {
    lcd_write_nibble(data & 0xF0, REGISTER_SELECT);
    lcd_write_nibble((data << 4) & 0xF0, REGISTER_SELECT);
}

void lcd_init(void) {
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_write_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write_nibble(0x30, 0);
    ets_delay_us(150);
    lcd_write_nibble(0x30, 0);
    lcd_write_nibble(0x20, 0); 

    lcd_send_cmd(0x28); 
    lcd_send_cmd(0x0C); 
    lcd_send_cmd(0x06); 
    lcd_send_cmd(0x01); 
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t row_offsets[] = {0x00, 0x40};
    lcd_send_cmd(0x80 | (col + row_offsets[row]));
}

void lcd_print(const char *str) {
    while (*str) {
        lcd_send_data((uint8_t)(*str++));
    }
}

// HSR04 ULTRASONIK
float get_distance_cm(void) {
    gpio_set_level(TRIG_PIN, 0);
    ets_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    ets_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    int64_t start_wait = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - start_wait) > 25000) {
            return -1.0f;
        }
    }

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - echo_start) > 30000) {
            return -1.0f;
        }
    }

    int64_t echo_end = esp_timer_get_time();
    return (float)(echo_end - echo_start) * 0.0343f / 2.0f;
}

void app_main(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    lcd_init();

    // Konfigurasi GPIO Output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TRIG_PIN) | (1ULL << BUZZER_PIN) | 
                        (1ULL << LED_MERAH) | (1ULL << LED_KUNING) | (1ULL << LED_HIJAU),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Konfigurasi GPIO Input (ECHO)
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&echo_conf);

    // Matikan indikator awal
    gpio_set_level(LED_HIJAU, 0);
    gpio_set_level(LED_KUNING, 0);
    gpio_set_level(LED_MERAH, 0);
    gpio_set_level(BUZZER_PIN, 0);

    char line_buf[17];

    while (1) {
        float distance = get_distance_cm();

        if (distance < 0) {
            printf("Sensor Error\n");
            gpio_set_level(LED_HIJAU, 0);
            gpio_set_level(LED_KUNING, 0);
            gpio_set_level(LED_MERAH, 0);
            gpio_set_level(BUZZER_PIN, 0);

            // Tampilan LCD saat error
            lcd_set_cursor(0, 0);
            lcd_print("Jarak: Error    ");
            lcd_set_cursor(0, 1);
            lcd_print("Status: UNKNOWN ");
        } else {
            // Tampilkan Jarak di Baris 1 LCD
            snprintf(line_buf, sizeof(line_buf), "Jarak: %.1f cm   ", distance);
            lcd_set_cursor(0, 0);
            lcd_print(line_buf);

            // Cek Status dan update Baris 2 LCD
            if (distance > 150.0f) {
                // Status AMAN
                lcd_set_cursor(0, 1);
                lcd_print("Status: AMAN    ");

                gpio_set_level(LED_HIJAU, 1);
                gpio_set_level(LED_KUNING, 0);
                gpio_set_level(LED_MERAH, 0);
                gpio_set_level(BUZZER_PIN, 0);
            } 
            else if (distance <= 150.0f && distance >= 50.0f) {
                // Status SIAGA
                lcd_set_cursor(0, 1);
                lcd_print("Status: SIAGA   ");

                gpio_set_level(LED_HIJAU, 0);
                gpio_set_level(LED_KUNING, 1);
                gpio_set_level(LED_MERAH, 0);
                gpio_set_level(BUZZER_PIN, 0);
            } 
            else {
                // Status BAHAYA
                lcd_set_cursor(0, 1);
                lcd_print("Status: BAHAYA! ");

                gpio_set_level(LED_HIJAU, 0);
                gpio_set_level(LED_KUNING, 0);
                gpio_set_level(LED_MERAH, 1);
                
                // Beep intermiten
                gpio_set_level(BUZZER_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(BUZZER_PIN, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}