#ifndef _HWCONFIG_H_
#define _HWCONFIG_H_

// Motor control
#define HWCONFIG_STEPPER_DIR_PIN GPIO_NUM_33
#define HWCONFIG_STEPPER_STEP_PIN GPIO_NUM_25
#define HWCONFIG_STEPPER_SLP_PIN GPIO_NUM_26

// Hall sensor to detect home position
#define HWCONFIG_HOMESENSOR_PIN GPIO_NUM_32

// Servo-motor
#define HWCONFIG_SERVOMOTOR_PIN GPIO_NUM_18

// Ramp led control
#define HWCONFIG_RAMPLED_PIN GPIO_NUM_23

// External button (input only)
#define HWCONFIG_UIBUTTON_PIN GPIO_NUM_35

// Wormhole LEDs
#define HWCONFIG_WORMHOLELEDS_PIN GPIO_NUM_19
#define HWCONFIG_WORMHOLELEDS_RMTCHANNEL RMT_CHANNEL_0
#define HWCONFIG_WORMHOLELEDS_LEDCOUNT 48

// Mp3 music player
#define HWCONFIG_MP3PLAYER_PORT_NUM UART_NUM_2
#define HWCONFIG_MP3PLAYER_BUFFSIZE 128
#define HWCONFIG_MP3PLAYER_BAUDRATE 115200
#define HWCONFIG_MP3PLAYER_RX2TXD GPIO_NUM_16
#define HWCONFIG_MP3PLAYER_TX2RXD GPIO_NUM_17

// I2C
#define HWCONFIG_I2C_MASTER_NUM 0
#define HWCONFIG_I2C_MASTER_FREQ_HZ 100000
#define HWCONFIG_I2C_SCL GPIO_NUM_22
#define HWCONFIG_I2C_SDA GPIO_NUM_21
#define HWCONFIG_I2C_MASTER_TX_BUF_DISABLE 0
#define HWCONFIG_I2C_MASTER_RX_BUF_DISABLE 0

#define HWCONFIG_OLED_ISPRESENT (1)
#define HWCONFIG_OLED_ADDR (1)

// Sanity led
#define HWCONFIG_SANITY_PIN GPIO_NUM_5

// OLED screen
#define HWCONFIG_OLED_I2CADDR (0x3C)


#endif