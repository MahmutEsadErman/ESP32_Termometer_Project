#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

// Function declarations
esp_err_t i2c_master_init(void);
esp_err_t lcd_init(void);
esp_err_t lcd_set_cursor(uint8_t row, uint8_t col);
esp_err_t lcd_print_string(const char* str);
esp_err_t lcd_clear(void);

// I2C configuration
#define I2C_MASTER_SCL_IO           22    // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           21    // GPIO number for I2C master data
#define I2C_MASTER_FREQ_HZ          100000 // I2C master clock frequency
#define I2C_MASTER_TIMEOUT_MS       1000

// Global I2C master handle
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t i2c_dev_handle = NULL;

// LCD configuration
#define LCD_I2C_ADDRESS             0x27  // Common I2C address for PCF8574 I2C backpack
#define LCD_ROWS                    2     // Number of rows
#define LCD_COLS                    16    // Number of columns

// LCD Commands (HD44780 compatible)
#define LCD_CMD_CLEAR_DISPLAY       0x01
#define LCD_CMD_RETURN_HOME         0x02
#define LCD_CMD_ENTRY_MODE_SET      0x04
#define LCD_CMD_DISPLAY_CONTROL     0x08
#define LCD_CMD_CURSOR_SHIFT        0x10
#define LCD_CMD_FUNCTION_SET        0x20
#define LCD_CMD_SET_CGRAM_ADDR      0x40
#define LCD_CMD_SET_DDRAM_ADDR      0x80

// Entry mode flags
#define LCD_ENTRY_RIGHT             0x00
#define LCD_ENTRY_LEFT              0x02
#define LCD_ENTRY_SHIFT_INCREMENT   0x01
#define LCD_ENTRY_SHIFT_DECREMENT   0x00

// Display control flags
#define LCD_DISPLAY_ON              0x04
#define LCD_DISPLAY_OFF             0x00
#define LCD_CURSOR_ON               0x02
#define LCD_CURSOR_OFF              0x00
#define LCD_BLINK_ON                0x01
#define LCD_BLINK_OFF               0x00

// Function set flags
#define LCD_8BIT_MODE               0x10
#define LCD_4BIT_MODE               0x00
#define LCD_2LINE                   0x08
#define LCD_1LINE                   0x00
#define LCD_5x10DOTS                0x04
#define LCD_5x8DOTS                 0x00

// PCF8574 I2C backpack bit mapping
#define LCD_BACKLIGHT               0x08
#define LCD_ENABLE                  0x04
#define LCD_READ_WRITE              0x02
#define LCD_REGISTER_SELECT         0x01

// I2C write function for LCD
static esp_err_t lcd_write_i2c(uint8_t data) {
    return i2c_master_transmit(i2c_dev_handle, &data, 1, I2C_MASTER_TIMEOUT_MS);
}

// Write 4 bits to LCD
static esp_err_t lcd_write_4bits(uint8_t data) {
    uint8_t buf = data | LCD_BACKLIGHT;
    esp_err_t ret;
    
    // Write data with enable high
    ret = lcd_write_i2c(buf | LCD_ENABLE);
    if (ret != ESP_OK) return ret;
    
    vTaskDelay(pdMS_TO_TICKS(1)); // Enable pulse width
    
    // Write data with enable low
    ret = lcd_write_i2c(buf & ~LCD_ENABLE);
    if (ret != ESP_OK) return ret;
    
    vTaskDelay(pdMS_TO_TICKS(1)); // Commands need > 37us to settle
    
    return ESP_OK;
}

// Write 8 bits to LCD (as two 4-bit operations)
static esp_err_t lcd_write_8bits(uint8_t data, uint8_t mode) {
    uint8_t high_nibble = (data & 0xF0) | mode;
    uint8_t low_nibble = ((data << 4) & 0xF0) | mode;
    
    esp_err_t ret = lcd_write_4bits(high_nibble);
    if (ret != ESP_OK) return ret;
    
    return lcd_write_4bits(low_nibble);
}

// Send command to LCD
static esp_err_t lcd_send_command(uint8_t cmd) {
    return lcd_write_8bits(cmd, 0); // RS = 0 for command
}

// Send data to LCD
static esp_err_t lcd_send_data(uint8_t data) {
    return lcd_write_8bits(data, LCD_REGISTER_SELECT); // RS = 1 for data
}

// Initialize I2C master
esp_err_t i2c_master_init() {
    // Configure I2C master bus
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Configure I2C device
    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    return i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &i2c_dev_handle);
}

// Initialize LCD
esp_err_t lcd_init() {
    esp_err_t ret;
    
    // Wait for LCD to power up
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Initialize in 4-bit mode
    // Send 0x03 three times to ensure 8-bit mode is cleared
    ret = lcd_write_4bits(0x30);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(5));
    
    ret = lcd_write_4bits(0x30);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(1));
    
    ret = lcd_write_4bits(0x30);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Switch to 4-bit mode
    ret = lcd_write_4bits(0x20);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Function set: 4-bit mode, 2 lines, 5x8 font
    ret = lcd_send_command(LCD_CMD_FUNCTION_SET | LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS);
    if (ret != ESP_OK) return ret;
    
    // Display control: display on, cursor off, blink off
    ret = lcd_send_command(LCD_CMD_DISPLAY_CONTROL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
    if (ret != ESP_OK) return ret;
    
    // Clear display
    ret = lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(2)); // Clear command takes longer
    
    // Entry mode: cursor moves right, no shift
    ret = lcd_send_command(LCD_CMD_ENTRY_MODE_SET | LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT);
    if (ret != ESP_OK) return ret;
    
    return ESP_OK;
}

// Set cursor position
esp_err_t lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40}; // Row 0 starts at 0x00, Row 1 starts at 0x40
    
    if (row >= LCD_ROWS) {
        row = LCD_ROWS - 1;
    }
    if (col >= LCD_COLS) {
        col = LCD_COLS - 1;
    }
    
    return lcd_send_command(LCD_CMD_SET_DDRAM_ADDR | (col + row_offsets[row]));
}

// Print string to LCD
esp_err_t lcd_print_string(const char* str) {
    esp_err_t ret = ESP_OK;
    
    while (*str && ret == ESP_OK) {
        ret = lcd_send_data(*str);
        str++;
    }
    
    return ret;
}

// Clear LCD display
esp_err_t lcd_clear(void) {
    esp_err_t ret = lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(2)); // Clear command takes longer
    }
    return ret;
}