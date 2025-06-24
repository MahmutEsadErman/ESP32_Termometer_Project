#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_OUTPUT_RED         19
#define LEDC_OUTPUT_GREEN       18
#define LEDC_OUTPUT_BLUE        5
#define LEDC_CHANNEL_RED        LEDC_CHANNEL_0
#define LEDC_CHANNEL_GREEN      LEDC_CHANNEL_1
#define LEDC_CHANNEL_BLUE       LEDC_CHANNEL_2
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          5000

void configure_led_pwm()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t channels[3] = {
        {
            .channel    = LEDC_CHANNEL_RED,
            .duty       = 0,
            .gpio_num   = LEDC_OUTPUT_RED,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        },
        {
            .channel    = LEDC_CHANNEL_GREEN,
            .duty       = 0,
            .gpio_num   = LEDC_OUTPUT_GREEN,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        },
        {
            .channel    = LEDC_CHANNEL_BLUE,
            .duty       = 0,
            .gpio_num   = LEDC_OUTPUT_BLUE,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        }
    };

    for (int i = 0; i < 3; i++) {
        ledc_channel_config(&channels[i]);
    }
}

static void rgb_set_color(uint8_t red, uint8_t green, uint8_t blue){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RED, red);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RED);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_GREEN, green);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_GREEN);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_BLUE, blue);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_BLUE);
}

void temperature2rgb(int8_t temp){
  temp = (temp + 24) * 255 / 104;
  rgb_set_color(temp, 40, 255-temp);
}