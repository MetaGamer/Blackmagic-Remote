#include "HFButtons.h"
#include "wiring_private.h"
#include "pins_arduino.h"



/*************************************************************************
 *  Momentary push buttons
 */

// These buttons will return 'active' for exactly 1 polling cycle for each
// time they are pressed and released. The 'active' is returned as soon as
// the button is pressed, and will not be true again until the button has
// been released.


HF_button::HF_button(int pin)
{
  pinMode(pin, INPUT_PULLUP);
  _pin = pin;
  _state = inactive;
}


//
// returns true after the button has been pressed
//   AND then released
//

int HF_button::active()
{
  if (_state == pressed)
  {
    _state = polled;
    return true;
  }
  return false;
}

void HF_button::update()
{
  int button_pressed = !digitalRead(_pin); // active low signal

  switch (_state)
  {
    case inactive:
      if (button_pressed)
        _state = pressed;
      break;
    case pressed:
      // do nothing- this state change happens in a different function
      break;
    case polled:
      if (!button_pressed)
        _state = unpressed;
      break;
    case unpressed:
      _state = inactive;
  }
}



/*************************************************************************
 *  Sliders (potentiometers)
 */

HF_slider::HF_slider(int pin)
{
  // pinMode(pin, INPUT);
  _pin = pin;
  _min = 0;
  _max = 1023;
  _lastvalue = 512;
  _mindelta = 5;
}


//
// returns true after the slider has changed enough past a threshold,
//   to avoid edge conditions where it might oscillate between 2 values
//

int HF_slider::changed()
{
  if (abs(_value - _lastvalue) > _mindelta)
  {
    _lastvalue = _value;
    return true;
  }
  return false;
}

int HF_slider::value()
{
  return _value;
}

/*************************************************************************
 *  Functions to read analog inputs without blocking
 *  Shamelessly adapted from the Arduino core.
 */

void HF_slider::start_read()
{
	uint8_t pin = _pin;
	const uint8_t analog_reference = DEFAULT;

	#if defined(analogPinToChannel)
	#if defined(__AVR_ATmega32U4__)
		if (pin >= 18) pin -= 18; // allow for channel or pin numbers
	#endif
		pin = analogPinToChannel(pin);
	#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
		if (pin >= 54) pin -= 54; // allow for channel or pin numbers
	#elif defined(__AVR_ATmega32U4__)
		if (pin >= 18) pin -= 18; // allow for channel or pin numbers
	#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
		if (pin >= 24) pin -= 24; // allow for channel or pin numbers
	#else
		if (pin >= 14) pin -= 14; // allow for channel or pin numbers
	#endif

	#if defined(ADCSRB) && defined(MUX5)
		// the MUX5 bit of ADCSRB selects whether we're reading from channels
		// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
		ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
	#endif

		// set the analog reference (high two bits of ADMUX) and select the
		// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
		// to 0 (the default).
	#if defined(ADMUX)
	#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
		ADMUX = (analog_reference << 4) | (pin & 0x07);
	#else
		ADMUX = (analog_reference << 6) | (pin & 0x07);
	#endif
	#endif


	// start the conversion
	#if defined(ADCSRA) && defined(ADCL)
		sbi(ADCSRA, ADSC);
	#endif

}

bool A2D_complete()
{
	// ADSC is cleared when the conversion finishes
	if (bit_is_set(ADCSRA, ADSC))
		return 0;
	else
		return 1;
}

void HF_slider::complete_read()
{
	uint8_t low, high;
	#if defined(ADCSRA) && defined(ADCL)
		// we have to read ADCL first; doing so locks both ADCL
		// and ADCH until ADCH is read.  reading ADCL second would
		// cause the results of each conversion to be discarded,
		// as ADCL and ADCH would be locked when it completed.
		low  = ADCL;
		high = ADCH;
	#else
		// we don't have an ADC, return 0
		low  = 0;
		high = 0;
	#endif

	// combine the two bytes
	_value = (high << 8) | low;

}
