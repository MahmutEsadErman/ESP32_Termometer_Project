#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

// My libraries
#include "lcd.c"
#include "rgb_led.c"

// RGB LED GPIO pins
#define LED_RED_GPIO    GPIO_NUM_25
#define LED_GREEN_GPIO  GPIO_NUM_26
#define LED_BLUE_GPIO   GPIO_NUM_27

// LEDC configuration
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

// LEDC channels for RGB
#define LEDC_CHANNEL_RED        LEDC_CHANNEL_0
#define LEDC_CHANNEL_GREEN      LEDC_CHANNEL_1
#define LEDC_CHANNEL_BLUE       LEDC_CHANNEL_2

// Maximum duty cycle value (2^13 - 1 = 8191 for 13-bit resolution)
#define LEDC_DUTY_MAX           ((1 << LEDC_DUTY_RES) - 1)

// NTC Termometer configuration
#define NTC_ADC_CHANNEL    ADC_CHANNEL_6  // GPIO34
#define NTC_ADC_ATTEN      ADC_ATTEN_DB_12
#define BETA               3950.0
#define NOMINAL_TEMP_K     298.15
#define ADC_MAX            4095.0

#define TAG "MAIN"

// Button configuration
#define BTN_PIN GPIO_NUM_13

// Globals
static SemaphoreHandle_t temp_sem = NULL;
static SemaphoreHandle_t btn_sem = NULL;
float temp_c;
adc_oneshot_unit_handle_t adc1_handle;

// Timer handle
esp_timer_handle_t temp_timer;

static void IRAM_ATTR btn_isr_handler(void* arg) {
  xSemaphoreGiveFromISR(btn_sem, NULL);
}

// Temperature reading task
void read_temperature(void* arg)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, NTC_ADC_CHANNEL, &raw));

    // Avoid division by 0
    if (raw <= 0 || raw >= ADC_MAX) return;

    temp_c = 1.0 / (log(1.0 / (ADC_MAX / (float)raw - 1.0)) / BETA + 1.0 / NOMINAL_TEMP_K) - 273.15;

    xSemaphoreGive(temp_sem);
    xSemaphoreGive(temp_sem);
}

// Lcd task
static void lcd_task(void *pvParameters) {
  float seconds;
  char buffer[32];
  uint8_t btn_state = 0;
  
  while (1) {
    if(xSemaphoreTake(btn_sem, 0) == pdTRUE) {
      btn_state = !btn_state;
      lcd_clear();
    }

    if (btn_state){ // Second Screen
        // First line
        lcd_set_cursor(0, 0);
        lcd_print_string("Seconds passed:");
        
        // Second line
        // Get seconds as a float
        seconds = esp_timer_get_time() / 1000000.0;
        lcd_set_cursor(1, 0);
        sprintf(buffer, "%.2fs", seconds);
        lcd_print_string(buffer);
    }
    else{ // Temperature Screen
      if (xSemaphoreTake(temp_sem, 10 / portTICK_PERIOD_MS) == pdTRUE) {
          // First line
          lcd_set_cursor(0, 0);
          lcd_print_string("Temperature:");
          lcd_set_cursor(1, 0);
          sprintf(buffer, "%.2fC", temp_c);
          lcd_print_string(buffer);
      }    
    }
  }
}

// RGB LED task
static void led_task(void *pvParameters) {
  while(1){
    if (xSemaphoreTake(temp_sem, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        temperature2rgb(temp_c);
    } 
  }  
}

void button_config(){
    // Install isr service
    gpio_install_isr_service(0);
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(BTN_PIN);
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(BTN_PIN, btn_isr_handler, NULL);
    ESP_LOGI(TAG, "Button configured successfully");
}

void app_main(void){
  // Create queues and Semaphores
  temp_sem = xSemaphoreCreateCounting(2, 0);
  btn_sem = xSemaphoreCreateBinary();

  // Configure Button
  button_config();

  // Configure rgb LED
  configure_led_pwm();

  // Configure ADC
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_12,
      .atten = NTC_ADC_ATTEN,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, NTC_ADC_CHANNEL, &config));

  // Timer configuration
  const esp_timer_create_args_t timer_args = {
      .callback = &read_temperature,
      .name = "ntc_timer"
  };

  // Initialize I2C connection to for lcd
  ESP_ERROR_CHECK(i2c_master_init());
  ESP_LOGI(TAG, "I2C initialized successfully");
  
  // Initialize LCD
  ESP_ERROR_CHECK(lcd_init());
  ESP_LOGI(TAG, "LCD initialized successfully");
  
  // Create lcd task
  xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 1, NULL);

  // Create led task
  xTaskCreate(led_task, "led_task", 1024, NULL, 1, NULL);
  
  // Create timer to fetch data from ntc termometer
  esp_timer_create(&timer_args, &temp_timer);
  esp_timer_start_periodic(temp_timer, 100000);  // 1 second = 1,000,000 µs

}
