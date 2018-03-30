#ifndef TESTPINS
#define TESTPINS

#include <Arduino.h>

#define NOP __asm__ __volatile__("nop")

#define TESTPIN12BIT (1<<(12-8))
#define TESTPIN13BIT (1<<(13-8))

#define TESTPIN12 PORTB & TESTPIN12BIT
#define TESTPIN13 PORTB & TESTPIN13BIT

void set_pin_12();
void set_pin_13();
void unset_pin_12();
void unset_pin_13();

#endif

