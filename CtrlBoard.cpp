
#include <Arduino.h>
#include "HFButtons.h"
#include "CtrlBoard.h"
#include "S.Bus.h"

HF_button  *CTRL_button[CTRL_NUM_BUTTONS];
const char *CTRL_button_name[] = CTRL_BOARD_BUTTON_NAMES ;
const int   CTRL_button_pin[]  = CTRL_BOARD_BUTTON_PINS ;

HF_slider  *CTRL_slider[CTRL_NUM_SLIDERS];
const int   CTRL_slider_pin[]  = CTRL_BOARD_SLIDER_NAMES ;
const char *CTRL_slider_name[] = CTRL_BOARD_SLIDER_PINS ;



//////////////////////////////////////////////////////////////////////////////
// Create the arrays of input objects.
//

void CTRL_setup()
{
  pinMode(SBUS_PIN, OUTPUT);
  pinMode(ORANGE_PIN, OUTPUT);
  return;

  for (int i = 0; i < CTRL_NUM_BUTTONS; i++)
    CTRL_button[i] = new HF_button(CTRL_button_pin[i]);

  for (int i = 0; i < CTRL_NUM_SLIDERS; i++)
    CTRL_slider[i] = new HF_slider(CTRL_slider_pin[i]);
}


//////////////////////////////////////////////////////////////////////////////
// State machine for polling the inputs of the board.
// The main purpose is to spread the load of polling inputs over many interrupt cycles.
//
// Each button only needs to be updated at 200ms to achieve "instantaneous" feel.
// Higher or lower is ok but may waste cycles or feel 'laggy', depending.
// To achieve this, polling at faster than 5Hz is desirable because the whole feedback
// loop needs to be achieved in 200ms, including updating the display to the user.
//
// The IRQs in this system are happening at 100KHz. If we just cycled through
// each button and polled them one per cycle, we'd update each button thousands of times
// faster than is actually necessary.
// NOTE: this might change when "animation" is added to e.g. pan from a preset A to B.
//
// This is accomplished via three methods:
// 1. cycle through each input, and update only one per cycle
// 2. for analog inputs, split the A2D conversion start and read into different cycles.
// 3. put dead cycles between some of the button updates so as to more evenly spread the
//    the ISRs during which work is being done.
//#if 1

typedef enum
{
  buttons,
  idle_after_button,
  slider_start,
  idle_after_slider_start,
  slider_read,
  idle_after_slider_read,
} CTRL_sequence;

#define CTRL_UPDATE_HZ 20

// This calculates how many idle cycles (of the CTRL state machine) to insert between
// each button update. It will likely not give an exact polling frequency of CTRL_UPDATE_HZ
// due to rounding errors, but that update rate is not critical.
#define CTRL_IRQS_PER_POLLING_CYCLE       (SBUS_HZ / CTRL_UPDATE_HZ)
#define CTRL_WORK_CYCLES                  (CTRL_NUM_BUTTONS + CTRL_NUM_SLIDERS + CTRL_NUM_SLIDERS)
#define CTRL_IDLE_IRQS_PER_POLLING_CYCLE  (CTRL_IRQS_PER_POLLING_CYCLE - CTRL_WORK_CYCLES)
#define CTRL_IDLE_IRQS_PER_WORK_UNIT      (int) (CTRL_IDLE_IRQS_PER_POLLING_CYCLE / CTRL_WORK_CYCLES)

int CTRL_input_number = 0;
int CTRL_cycles_idle  = 0;

CTRL_sequence CTRL_polling_state = buttons;

void CTRL_update()
{
	switch (CTRL_polling_state)
	{
	  case buttons:
		  CTRL_button[CTRL_input_number]->update();
		  CTRL_input_number++;
		  CTRL_polling_state = idle_after_button;
		  CTRL_cycles_idle  = 0;
		  break;
	  case idle_after_button:
		  CTRL_cycles_idle++;
		  if (CTRL_cycles_idle >= CTRL_IDLE_IRQS_PER_WORK_UNIT)
		  {
		    if (CTRL_input_number < CTRL_NUM_BUTTONS)
		      {
		    	CTRL_polling_state = buttons;
		      }
		    else
		      {
  		    	CTRL_polling_state = slider_start;
  		    	CTRL_input_number = 0;
		      }
		  }
		  break;
	  case slider_start:
		  CTRL_slider[CTRL_input_number]->start_read();
		  CTRL_polling_state = idle_after_slider_start;
		  CTRL_cycles_idle  = 0;
		  break;
	  case idle_after_slider_start:
		  CTRL_cycles_idle++;
		  if ((CTRL_cycles_idle >= CTRL_IDLE_IRQS_PER_WORK_UNIT) && A2D_complete())
		  {
		    CTRL_polling_state = slider_read;
		  }
		  break;
	  case slider_read:
		  CTRL_slider[CTRL_input_number]->complete_read();
		  CTRL_input_number++;
		  CTRL_polling_state = idle_after_slider_start;
		  CTRL_cycles_idle  = 0;
		  break;
	  case idle_after_slider_read:
		  CTRL_cycles_idle++;
		  if (CTRL_cycles_idle >= CTRL_IDLE_IRQS_PER_WORK_UNIT)
		  {
		  	if (CTRL_input_number < CTRL_NUM_SLIDERS)
  			{
  				CTRL_polling_state = slider_start;
  			}
  			else
  		  {
  				CTRL_polling_state = buttons;
  				CTRL_input_number = 0;
  		  }
  	  }
		  break;
  }
}
