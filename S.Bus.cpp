/*
 * (c) Michael Lewis, 2018
 * Thanks to Amanda Ghassaei for the interrupt code example.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
*/


#include <Arduino.h>
#include "S.Bus.h"
#include "CtrlBoard.h"

/***********************************************************************************************
 * S.Bus data structure
 *
 * The S.Bus protocol transmits 16 channels of 11-bit information plus 1 byte of "flags".
 * The 11 * 16 =  176 bits of channel info is transmitted in 22 8-bit bytes (22 * 8 = 176 bits).
 * The flags byte include two 'digital channels' as single bits for a total of 18 "channels"
 * per the S.Bus terminology.
 *
 * This whole data structure is wrapped in a pre-defined fixed-value start byte and end byte.
 * These are used by the receiver to ensure the whole data structure was captured properly.
 *
 * On top of all that, each byte sent is wrapped in the 8E2 protocol, which means there is
 * actually 11 bits sent for each 8-bit byte of data. But that happens in the serial IRQ
 * protocol above. (Not to be confused with splitting each 11-bit channel over multiple 8-bit
 * data bytes.
 *
 * The data here is double-buffered. It gets copied from the "next" to the "current" buffer
 * at the end of the whole 25-byte sequence to prevent sending half-changed data.
 *
 ************************************************************************************************
 *
 * The following S.Bus definition was copied from
 * https://os.mbed.com/users/Digixx/notebook/futaba-s-bus-controlled-by-mbed/
 *
 * S.Bus protocol (in 8 bit bytes):
 * [startbyte] [data1] [data2] .... [data22] [flags][endbyte]
 *
 * startbyte = 0xF0
 * data 1-22 = [ch1, 11bit][ch2, 11bit] .... [ch16, 11bit]
 * channel 1 uses 8 bits from data1 and 3 bits from data2
 * channel 2 uses last 5 bits from data2 and 6 bits from data3
 * ...
 * channel 16 uses last 3 bits from data21 and 8 bits from data22
 * flags byte:
 *   bit7 = ch17 = digital channel (0x80)
 *   bit6 = ch18 = digital channel (0x40)
 *   bit5 = Frame lost, equivalent red LED on receiver (0x20)
 *   bit4 = failsafe activated (0x10)
 *   bit3 = n/a
 *   bit2 = n/a
 *   bit1 = n/a
 *   bit0 = n/a
 *  endbyte = 0x00
 *
 *****************************************************************************
 */


// These track the state of the current packet being sent.
// The overall packet structure is tracked via the 'state' enum.
// Each interrupt sends one bit, so we track which bit.
// When sending channel data (the whole point), we track the channel we are on.

SBUS_sequence  SBUS_state         = startbyte;
int            SBUS_state_bit     = 0;
int            SBUS_state_channel = 0;

// This is the buffer currently being sent, and the one that can be modified.
// Once per packet cycle the 'next' buffer is copied to the channel buffer.
int SBUS_channel[SBUS_CH_COUNT];
int SBUS_next_channel[SBUS_CH_COUNT];

//////////////////////////////////////////////////////////////////////////////
// Gotta initialize those buffers!

void SBUS_setup()
{
  for (int i = 0; i < SBUS_CH_COUNT; i++)
  {
    SBUS_channel[i]      = 0;
    SBUS_next_channel[i] = 0;
  }

  // Set up the S.BUs pin
  pinMode(SBUS_PIN, OUTPUT);

}

//////////////////////////////////////////////////////////////////////////////
// Calculate the value of the next bit to send. Note: this is just the
// 'data' part. It does not include the start, stop and parity bits. Those
// are added via another wrapper function.
// This also does some 'side-effect' work as we go through the packet-
// namely copying the 'next' channel value to the working buffer after each
// channel has been sent.
//


unsigned char SBUS_nextdatabit ()
{
  int databit;

  switch (SBUS_state)
  {
    case startbyte:
      databit = (SBUS_STARTBYTE >> SBUS_state_bit) & 0x01;
      SBUS_state_bit++;
      if (SBUS_state_bit > 7)
      {
        SBUS_state = channels;
        SBUS_state_channel = 0;
        SBUS_state_bit = 0;
      }
      break;
    case channels:
      databit = (SBUS_channel[SBUS_state_channel] >> SBUS_state_bit) & 0x01;
      SBUS_state_bit++;

      // there are 11 bits per channel in the S.Bus protocol
      if (SBUS_state_bit > 10)
      {
    	// Now that we've sent the whole channel, we can copy the next value
    	// from the "next" buffer.
    	// This may cause some channel data to not stay "in-sync": if channels X & Y
    	// are modified at the same time, channel X might get sent in packet N and
    	// channel Y in packet N+1. This should not be a problem for the kind of
    	// of data we are sending, but it is worth noting. Correcting this would
    	// be a little involved, but not awful. The obvious solution would be to
    	// copy the buffer at the end of the packet, but that copy might take longer
    	// than a single interrupt cycle.
        SBUS_channel[SBUS_state_channel] = SBUS_next_channel[SBUS_state_channel];

        // now we can move on to the net channel
        SBUS_state_channel++;
        SBUS_state_bit = 0;
        if (SBUS_state_channel >= SBUS_CH_COUNT)
        {
          // all channels are done... move to the end byte
          SBUS_state = endbyte;
        }
      }
      break;
    case endbyte:
      databit = (SBUS_ENDBYTE >> SBUS_state_bit) & 0x01;
      if (SBUS_state_bit > 7)
      {
        // this frame is over. Set up the next one.
        SBUS_state     = startbyte;
        SBUS_state_bit = 0;
      }
      break;
    default:
      databit = 0; // mostly to avoid compiler warnings
  }
  return databit;
}

/**************************************************************************************
 * SBUS_update_state
 *
 * This wraps data bytes in the proper serial protocol.
 * Each byte is sent over 12 bus clock cycles in "8E2" format:
 *   start             = 0
 *   data[0..7]        (This is the '8' in 8E2)
 *   parity            (such that the sum of data[7:0] + parity is even, the 'E' in 8E2)
 *   two stop bits     = 1 (This is the '2' in 8E2)
 *
 *  The data bits are obtained from the serialized S.Bus data structure (see SBUS_nextdatabit()).
*/

typedef enum
{
  startbit,
  databyte,
  paritybit,
  stopbit1,
  stopbit2
} sbus_serial_phase;

sbus_serial_phase sbus_current_serial_phase    = startbit;
int               sbus_current_serial_bitcount = 0;
int               sbus_current_parity          = 0;

unsigned char SBUS_nextpinvalue = 0;

void SBUS_update_state()
{
	  switch (sbus_current_serial_phase)
	  {
	    case startbit:
	      SBUS_nextpinvalue = 0;
	      sbus_current_parity = 0;
	      sbus_current_serial_phase = databyte;
	      sbus_current_serial_bitcount = 0;
	      break;
	    case databyte:
	      SBUS_nextpinvalue = SBUS_nextdatabit() & 0x01;
	      sbus_current_parity = sbus_current_parity ^ SBUS_nextpinvalue; // ^ is XOR operator
	      sbus_current_serial_bitcount++;
	      if (sbus_current_serial_bitcount == 8)
	      {
	        sbus_current_serial_phase = paritybit;
	      }
	      break;
	    case paritybit:
	      SBUS_nextpinvalue = sbus_current_parity;
	      sbus_current_serial_phase = stopbit1;
	      break;
	    case stopbit1:
	      SBUS_nextpinvalue = 1;
	      sbus_current_serial_phase = stopbit2;
	      break;
	    case stopbit2:
	      SBUS_nextpinvalue = 1;
	      sbus_current_serial_phase = startbit;
	      break;
	  }
}



