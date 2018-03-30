#include "HF_ISR.h"
#include "CtrlBoard.h"
#include "S.Bus.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <Adafruit_ssd1306syp_fast.h>
#include "TestPins.h"

extern Adafruit_ssd1306syp_fast display;

/**************************************************************************************
 * IRQ timer setup.
 *   This will create an interrupt at a frequency of the S.Bus protocol,
 *   allowing us to easily send properly spaced bits.
 *
 *  This IRQ setup is for any Arduino uno or any board with ATMEL 328/168:
 *     such as diecimila, duemilanove, lilypad, nano, mini
 *
 *  TODO: check the architecture somehow during compile time.
 *  TODO: get the CPU freq during compile time.
 */

volatile int IRQ_count_high;
volatile int IRQ_count;

void HF_IRQ_setup()
{
  #if CPU_HZ > (SBUS_HZ * 255)
  #error ERROR: SBUS frequency out of bounds for timer compare register.
  #endif
// stop interrupts
cli();

  #if HF_IRQ_TIMER == 0
    // Note: IRQ_0 is used by Serial, so we can't use it and have terminal debugging

    //set timer0 interrupt at kHz based on CPU and S.Bus speed
    TCCR0A = 0;// set entire TCCR2A register to 0
    TCCR0B = 0;// same for TCCR2B
    TCNT0  = 0;//initialize counter value to 0

    // set compare match register
    OCR0A = HF_IRQ_CYCLES;

    // turn on CTC mode
    TCCR0A |= (1 << WGM01);
    // Set CS00 bit for no prescaler
    TCCR0B |= (1 << CS00);
    // enable timer compare interrupt
    TIMSK0 |= (1 << OCIE0A);

  #elif HF_IRQ_TIMER == 1

      TCCR1A = 0;// set entire TCCR1A register to 0
      TCCR1B = 0;// same for TCCR1B
      TCNT1  = 0;//initialize counter value to 0
      // set compare match register for 1hz increments
      OCR1A = HF_IRQ_CYCLES; //5; //6; //2; //4;// = (16*10^6) / (1*1024) - 1 (must be <65536)
      // turn on CTC mode
      TCCR1B |= (1 << WGM12);
      // Set CS10 and CS12 bits for 1024 prescaler
      // TCCR1B |= (1 << CS12) | (1 << CS10);
      // Set CS10 bit only for no prescaler
      // Set CS11 bit only for 8 prescaler
      // Set CS12 bit only for 256 prescaler
      TCCR1B |= (1 << CS10);
      // enable timer compare interrupt
      TIMSK1 |= (1 << OCIE1A);
  #elif HF_IRQ_TIMER == 2

    TCCR2A = 0;// set entire TCCR2A register to 0
    TCCR2B = 0;// same for TCCR2B
    TCNT2  = 0;//initialize counter value to 0

    // set compare match register
   OCR2A = HF_IRQ_CYCLES;
    // turn on CTC mode
    TCCR2A |= (1 << WGM21);
    // Set CS20 bit for 0 prescaler
    TCCR2B |=  (1 << CS20);
    // Also set CS21 bit for 32 prescaler ** SLOW IT DOWN FOR TESTING
    //TCCR2B |=  (1 << CS21);
    // enable timer compare interrupt
    TIMSK2 |= (1 << OCIE2A);

#else
#error Unsupported interrupt.
#endif

  IRQ_count_high = 0;
  IRQ_count = 0;

}

/**************************************************************************************
 * Interrupt Service Routine (ISR)
 *
 *   tl;dr: This ISR serves three functions.
 *   1. It drives the S.Bus at 10 KHz.
 *   2. It polls the input buttons at 100 Hz.
 *   3. It polls the analog inputs without blocking.
 *
 *   Long version:
 *   This ISR is also used to service multiple functions that in a different implementation
 *   would done with a separate IRQs. The objective here is to have only a single active
 *   IRQ so that we can have a very stable cycle time. Otherwise the IRQs interrupt
 *   each other leading to small, but problematic, variations in the timing of the
 *   inferred SBUS clock.
 *   1. It drives the S.Bus. This is the reason the IRQ is 10 KHz.
 *   2. It polls the input buttons. These should not be polled at more than 100 Hz
 *      or so for the purpose of de-bouncing.
 *   3. It polls the analog inputs. The Arduino library gives simple routines for
 *      reading them, but they waste a lot of cycles because using the A2D converter is a
 *      four-step process:
 *       1. select an input pin
 *       2. start conversion
 *       3. wait for the conversion flag to indicate that it's done
 *       4. read the digital value
 *      The Arduino library does all of these in one function call. Step 3 ("wait around")
 *      wastes potentially valuable clock cycles.
 *      This implementation does steps 1&2 during one SBus cycle, ands steps 3&4 during the next.
 *
 * 1. The S.Bus protocol is inverted, so all the bits are typically negated (~ operator)
 *    before writing to the pin
 * 2. To make the bus timing as close to uniform as possible, the pin state change is the first thing that happens.
 *    The value of the next pin state is then calculated and used on the next ISR call. As long as this calculation
 *    takes less time than the frequency of the interrupt (i.e. 100KHz) the bus frequency should be very stable.
 * 3. The one source of timing instability is to enable any other interrupt.
 *
 */

unsigned char temp_toggle = 0;

#if HF_IRQ_TIMER == 0
ISR(TIMER0_COMPA_vect)
#elif HF_IRQ_TIMER == 1
ISR(TIMER1_COMPA_vect)
#elif HF_IRQ_TIMER == 2
ISR(TIMER2_COMPA_vect)
#endif
{
  set_pin_13();
  unset_pin_13();
  set_pin_12();
/* Disable SBUS until control surface / OLED is working well.

  #if SBUS_ACTIVE_LOW
	digitalWrite(SBUS_PIN, (~SBUS_nextpinvalue) & 0x01);
  #else
	digitalWrite(SBUS_PIN, (SBUS_nextpinvalue) & 0x01);
  #endif

  SBUS_update_state();
*/

  // CTRL update does one small thing related to the board each cycle... often nothing, which
  // allows the main loop to do more processing.
  //CTRL_update();

  display.ISR_update(true); // TODO: enable for display to work

  /*
  set_pin_12();
  set_pin_13();
  unset_pin_13();
  unset_pin_12();
  */

  if (IRQ_count == 1000)
  {
    IRQ_count_high++;
    IRQ_count=0;
  }
  else
  {
    IRQ_count++;
  }

  unset_pin_12();
}
