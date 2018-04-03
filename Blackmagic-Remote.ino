/*
 * Notes:
 *
 * With two interrupts enabled the Arduino is not designed to create precisely timed real-time ISRs.
 */

#include <Adafruit_ssd1306syp_fast.h>
#include "S.Bus.h"
#include "CtrlBoard.h"
#include "HFButtons.h"
#include "HF_ISR.h"
#include "TestPins.h"

#include "avr/pgmspace.h"
#include "avr/io.h"

#define USE_OLED 1

Adafruit_ssd1306syp_fast display(OLED_SDA_PIN,OLED_SCL_PIN);

void setup()
{
  int i;
  //SBUS_setup();
  HF_IRQ_setup();
  CTRL_setup();

  // Wait for the OLED to power-up and reset
  sei();
  //delay(2000);
  //TIMSK0 = 0;
  display.initialize(); // IRQs must be enabled for the "fast" library to function (else it hangs).

  #if 0
    int i;
    Serial.begin(9600);

    Serial.print("Buttons available:\n");
    for (i = 0; i < CTRL_NUM_BUTTONS; i++)
    {
      Serial.println(CTRL_button_name[i]);
    }
    Serial.print("Sliders available:\n");
    for (i = 0; i < CTRL_NUM_SLIDERS; i++)
    {
      Serial.println(CTRL_slider_name[i]);
    }
    Serial.println("Setup complete.");
  #endif
/*
    display.clear();
    display.setTextSize(3);
    for (i = 0; i < CTRL_NUM_BUTTONS; i++)
    {
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.clear();
        display.println(CTRL_button_name[i]);
        display.update();
    }
*/
}

int loop_counter = 0;
int temp_count;
int old_IRQ_count=0;
int x = 0, y = 0;
int x_direction = 1;
int y_direction = 1;

void loop()
{
//  char cur_camera;

/*  set_pin_12();
  set_pin_13();
  unset_pin_13();
  unset_pin_12();
*/
  display.clear();
#if (USE_OLED)
  /*
    display.setCursor(0,32);
    for (int i=0; i<x;i++)
      display.print(i%10);

    for (int i = 0; i < CTRL_NUM_BUTTONS; i++)
    {
      if (CTRL_button[i]->active())
      {
        display.clear();
        display.setTextSize(4);
        display.setTextColor(WHITE);
        display.setCursor(0,16);
        display.println(CTRL_button_name[i]);
        break;
      }
    }

  for (int i = 0; i < CTRL_NUM_SLIDERS; i++)
    {
      if (CTRL_slider[i]->changed())
      {
        display.setTextSize(2);
        display.setTextColor(WHITE);

        display.setCursor(0,32);
        display.print(CTRL_slider_name[i]);
        display.print(" : ");
        display.println(CTRL_slider[i]->value());
        break;
      }
    }
*/
  display.setTextColor(WHITE);
  display.setTextSize(0);

/*  for (int rep=0;rep<10;rep++)
  {
    display.setCursor(0,32);
    for (int i=0; i<x;i++)
      display.print(i%10);
  }
*/
  display.setCursor(x,y);
  display.print(loop_counter++);

  // make it single digit
  if (loop_counter >= 10)
    loop_counter = 0;

  // Animate X,Y
  x += x_direction;
  if (x >= 64)
  {
    y+= 1;
    x_direction = -1;
  }
  else if (x <= 0) {
    y+= 1;
    x_direction = +1;
  }
  if (y >= 32)
  {
    y = 0;
  }

  // display IRQ count in fixed location

  display.setTextColor(WHITE);
  display.setTextSize(0);

  display.setCursor(60,64-7);
  temp_count = IRQ_count_high;
  display.print(temp_count);
  display.print("K");
  //display.setCursor(88,64-7);
  //temp_count = IRQ_count;
  //display.print(temp_count);

  //if (loop_counter==0)
    display.update();

	#endif

}
