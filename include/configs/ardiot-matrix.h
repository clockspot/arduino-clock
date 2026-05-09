// Arduino IoT (Nano 33 IoT) with MAX7219 (matrix), IMU control, no RTC
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDIOT_MATRIX_H
#define CONFIG_ARDIOT_MATRIX_H

#define ENABLE_ALARM2 false

#define RTC_MILLIS

#define INPUT_SIMPLE
#define INPUT_IMU
#define USB_DIR_UP
#define IC_DIR_BACK

#define DISPLAY_MAX7219
#define CLK_PIN 2 //D2, pin 20
#define CS_PIN 3  //D3, pin 21
#define DIN_PIN 4 //D4, pin 22

#endif //CONFIG_ARDIOT_MATRIX_H
