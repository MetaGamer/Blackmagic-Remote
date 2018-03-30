#include "TestPins.h"

void set_pin_12()
{
  PORTB = PORTB | TESTPIN12BIT;
}

void set_pin_13()
{
  PORTB = PORTB | TESTPIN13BIT;
}

void unset_pin_12()
{
  PORTB = PORTB & ~TESTPIN12BIT;
}

void unset_pin_13()
{
  PORTB = PORTB & ~TESTPIN13BIT;
}
