#ifndef HF_BUTTONS
#define HF_BUTTONS

#include "Arduino.h"
#include <avr/interrupt.h>


#define BMD_SBUS_min 44
#define BMD_SBUS_max 212
#define BMD_SBUS_mid 128

/*
typedef enum BMD_BMPCC_RangeControls { fstop, zoom, focus } bmd_range;
typedef enum BMD_BMPCC_ToggleControls { record, autofocus, gain, shutterangle, whitebalance, framerate } bmd_toggle;
*/


// pin is the input pin# to monitor
// _pressed is true iff the input was low last time it was sampled
// _pushed is true if the input toggles from low to high and then to low again


typedef enum
{
  inactive,
  pressed,
  polled,
  unpressed
} hf_pushbutton;


class HF_button
{
  public:
    HF_button(int pin);

    int           active();
    void          update();

  private:
    int                    _pin;
    volatile hf_pushbutton _state;


};

class HF_slider
{
  public:
    HF_slider(int pin);

    void          start_read();
	void          complete_read();
    int           changed();
    int           value();

  private:
    int           _pin;
    volatile int  _value;
    volatile int  _lastvalue;
    volatile int  _min;
    volatile int  _max;
    volatile int  _mindelta;

};


bool A2D_complete();

#endif // HF_BUTTONS
