#ifndef CTRL_BOARD
#define CTRL_BOARD

#include <Adafruit_ssd1306syp_fast.h>
#include "HFButtons.h"

//extern Adafruit_ssd1306syp_fast display;

// Note: Pins 0 and 1 are used  by the USB port

#define CTRL_BOARD_BUTTON_NAMES { "up", "#", "focus", "A", "C", "down", "B", "joyhat" }
#define CTRL_BOARD_BUTTON_PINS  {   2 ,   3,      4,   5,    6,      7,   8,  11  }

#define OLED_SCL_PIN  9
#define OLED_SDA_PIN 10
#define SBUS_PIN     12
#define ORANGE_PIN   13

#define CTRL_BOARD_SLIDER_NAMES { PIN_A0,  PIN_A1,   PIN_A2,   PIN_A3,   PIN_A4,  PIN_A5 }
#define CTRL_BOARD_SLIDER_PINS  { "action", "circle", "speed", "focus", "joy-x", "joy-y"}


void CTRL_setup();
void CTRL_update();


// Array of buttons
const int CTRL_NUM_BUTTONS = 8;

extern HF_button  *CTRL_button[CTRL_NUM_BUTTONS];
extern const int   CTRL_button_pin[];
extern const char *CTRL_button_name[];


const int CTRL_NUM_SLIDERS = 6;
extern HF_slider  *CTRL_slider[CTRL_NUM_SLIDERS];
extern const int   CTRL_slider_pin[];
extern const char *CTRL_slider_name[];


#endif
