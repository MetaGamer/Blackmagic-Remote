
#ifndef SBUS_H
#define SBUS_H

/*******************************************************
 * S.Bus definitions
 */

#define SBUS_CH_BITS   11
#define SBUS_CH_COUNT  16
#define SBUS_STARTBYTE 0xF0
#define SBUS_ENDBYTE   0x00

typedef enum
{
  startbyte,
  channels,
  endbyte
} SBUS_sequence;


/*******************************************************
 * IRQ and ISR definitions
 */

#define SBUS_HZ 100000
#define SBUS_ACTIVE_LOW 1

extern unsigned char SBUS_nextpinvalue;

void SBUS_setup();
void SBUS_update_state();

#endif
