#define SDA_PIN 12 //GPIO12 / 6
#define SCL_PIN 14 //GPIO14 / 5
#define USE_BUTTONS 0 // YES=1 OR NO=0 (Rotary Encoder)

#if USE_BUTTONS
  #define OK_PIN 2 //GPIO2 / PIN NO: 17 / PIN: D4
  #define CANCEL_PIN 4 //GPIO4 / PIN NO: 19 / PIN: D2
  #define UP_PIN 0 //GPIO0 / PIN NO: 18 / PIN: D3
  #define RIGHT_PIN 13 //GPIO13 / PIN NO: 7 / PIN: D7
  #define LEFT_PIN 5 //GPIO5 / PIN NO: 20 / PIN: D1
  #define DOWN_PIN 16 //GPIO16 / PIN NO: 4 / PIN: D0
#else
  #define KEY_A 1
  #define KEY_B 2
  #define KEY_C 3
#endif
#define LED_PIN_1 15 //GPIO15 / PIN NO: 16 / PIN: D8 ~PWM

#define LOOP_UPDATE_DELAY 500

#define splash_width  25
#define splash_height 11
static const unsigned char splash_bits [] U8X8_PROGMEM = {
  0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x4a, 0x00,
  0xe9, 0x4c, 0x4a, 0x00, 0xa5, 0xd2, 0x32, 0x01,
  0xe3, 0x52, 0x93, 0x00, 0xa5, 0x52, 0x62, 0x00,
  0x29, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00,
  0x7c, 0xe0, 0x40, 0x00, 0x82, 0x19, 0x80, 0x01,
  0x01, 0x06, 0x00, 0x00
};
struct led
{
    int pinNo = 0;
    int brightnessLevel = 100;
    enum blink_pattern{SOLID,SLOW,MEDIUM,FAST};
};

