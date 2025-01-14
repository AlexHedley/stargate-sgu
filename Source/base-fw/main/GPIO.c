#include "GPIO.h"
#include "FWConfig.h"
#include "Servo.h"
#include "Settings.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include <string.h>
#include "WhiteBox.h"
#include "HWConfig.h"
#include "driver/i2c.h"

#define TAG "GPIO"

static led_strip_handle_t led_strip;
static SSD1306_handle m_ssd1306;

void GPIO_Init()
{
    //install gpio isr service
    gpio_install_isr_service(0);

    // Sanity leds
    gpio_set_direction(HWCONFIG_WORMHOLELEDS_PIN, GPIO_MODE_OUTPUT);

    // Sanity leds
    gpio_set_direction(HWCONFIG_SANITY_PIN, GPIO_MODE_OUTPUT);

    // Ramp LEDs
    gpio_set_direction(HWCONFIG_RAMPLED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RAMPLED_PIN, false);

    // Stepper PINs
    gpio_set_direction(HWCONFIG_STEPPER_DIR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(HWCONFIG_STEPPER_STEP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(HWCONFIG_STEPPER_SLP_PIN, GPIO_MODE_OUTPUT);
    GPIO_StopStepper();

    // Servo PIN
    gpio_set_direction(HWCONFIG_SERVOMOTOR_PIN, GPIO_MODE_OUTPUT);
    // Initialize motor driver
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, HWCONFIG_SERVOMOTOR_PIN);
    mcpwm_config_t servo_config;
    servo_config.frequency = 50;  // Frequency = 1000Hz,
    servo_config.cmpr_a = 0;      // Duty cycle of PWMxA = 0
    servo_config.cmpr_b = 0;      // Duty cycle of PWMxb = 0
    servo_config.counter_mode = MCPWM_UP_COUNTER;
    servo_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &servo_config);    // Configure PWM0A & PWM0B with above settings
    // 2/20 ms * 0.5*0.5
    GPIO_StopClamp(); // Don't piss off the servo motor


    // Init ramp LED
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_12_BIT,
        .freq_hz          = 5000,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = HWCONFIG_RAMPLED_PIN,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    GPIO_SetRampLightPerc(false);

    // Initialize GPIO for home sensor
    gpio_set_direction(HWCONFIG_HOMESENSOR_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_HOMESENSOR_PIN);

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = HWCONFIG_WORMHOLELEDS_PIN,
        .max_leds = HWCONFIG_WORMHOLELEDS_LEDCOUNT, // sanity LED + at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);

    #ifndef WHITEBOX_SOUNDFX_DEACTIVATE
    // =====================
    // UART drive to drive the Mp3 player
        /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = HWCONFIG_MP3PLAYER_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    #if CONFIG_UART_ISR_IN_IRAM
        intr_alloc_flags = ESP_INTR_FLAG_IRAM;
    #endif

    ESP_ERROR_CHECK(uart_driver_install(HWCONFIG_MP3PLAYER_PORT_NUM, HWCONFIG_MP3PLAYER_BUFFSIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(HWCONFIG_MP3PLAYER_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HWCONFIG_MP3PLAYER_PORT_NUM, HWCONFIG_MP3PLAYER_TX2RXD, HWCONFIG_MP3PLAYER_RX2TXD, -1, -1));
    #endif

	const int i2c_master_port = HWCONFIG_I2C_MASTER_NUM;
    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = HWCONFIG_I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = HWCONFIG_I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = HWCONFIG_I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);

	ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, HWCONFIG_I2C_MASTER_RX_BUF_DISABLE, HWCONFIG_I2C_MASTER_TX_BUF_DISABLE, 0));

    #ifdef HWCONFIG_OLED_ISPRESENT
    static SSD1306_config cfgSSD1306 = SSD1306_CONFIG_DEFAULT_128x64;
	//cfgSSD1306.pinReset = (gpio_num_t)CONFIG_I2C_MASTER_RESET;
    SSD1306_Init(&m_ssd1306, i2c_master_port, &cfgSSD1306);
    SSD1306_ClearDisplay(&m_ssd1306);
    const char* szBooting = "Booting ...";
    SSD1306_DrawString(&m_ssd1306, 0, 0, szBooting, strlen(szBooting));
    SSD1306_UpdateDisplay(&m_ssd1306);
    #endif
}

void GPIO_SendMp3PlayerCMD(const char* szCmd)
{
    #ifndef WHITEBOX_SOUNDFX_DEACTIVATE
    uart_write_bytes(HWCONFIG_MP3PLAYER_PORT_NUM, (const char *)szCmd, strlen(szCmd));
    #endif
}

/*! @brief The gate spin counter-clockwise */
void GPIO_StepMotorCW()
{
    gpio_set_level(HWCONFIG_STEPPER_DIR_PIN, true);
    gpio_set_level(HWCONFIG_STEPPER_STEP_PIN, true);
    esp_rom_delay_us(10);
    gpio_set_level(HWCONFIG_STEPPER_STEP_PIN, false);
    esp_rom_delay_us(10);
}

/*! @brief The gate spin clockwise */
void GPIO_StepMotorCCW()
{
    gpio_set_level(HWCONFIG_STEPPER_DIR_PIN, false);
    gpio_set_level(HWCONFIG_STEPPER_STEP_PIN, true);
    esp_rom_delay_us(10);
    gpio_set_level(HWCONFIG_STEPPER_STEP_PIN, false);
    esp_rom_delay_us(10);
}

/*! @brief motor start */
void GPIO_StartStepper()
{
    gpio_set_level(HWCONFIG_STEPPER_SLP_PIN, true);
}

/*! @brief motor stop */
void GPIO_StopStepper()
{
    gpio_set_level(HWCONFIG_STEPPER_SLP_PIN, false);
}

void GPIO_ReleaseClamp()
{
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, SERVO_PWM2PERCENT(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_ClampReleasedPWM)));
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
    vTaskDelay(pdMS_TO_TICKS(600));
    // Stopping it prevent the annoying sound of the servo and save some power
    //mcpwm_set_signal_low(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A);
}

void GPIO_LockClamp()
{
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, SERVO_PWM2PERCENT(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_ClampLockedPWM)));
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
    vTaskDelay(pdMS_TO_TICKS(350));
    // Stopping it prevent the annoying sound of the servo and save some power
    mcpwm_set_signal_low(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A);
}

void GPIO_StopClamp()
{
    mcpwm_set_signal_low(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A);
}

bool GPIO_IsHomeActive()
{
    return !gpio_get_level(HWCONFIG_HOMESENSOR_PIN);
}

void GPIO_SetRampLightPerc(float fltPercent)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095 * fltPercent));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void GPIO_SetSanityLEDStatus(bool bIsLightUp)
{
    gpio_set_level(HWCONFIG_SANITY_PIN, !bIsLightUp);
}

void GPIO_SetPixel(uint32_t u32Index, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue)
{
    led_strip_set_pixel(led_strip, u32Index, u8Red, u8Green, u8Blue);
}

void GPIO_ClearAllPixels()
{
    led_strip_clear(led_strip);
}

void GPIO_RefreshPixels()
{
    led_strip_refresh(led_strip);
}

#ifdef HWCONFIG_OLED_ISPRESENT
SSD1306_handle* GPIO_GetSSD1306Handle()
{
    return &m_ssd1306;
}
#endif