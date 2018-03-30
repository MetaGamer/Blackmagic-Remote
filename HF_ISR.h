#ifndef HF_ISR
#define HF_ISR

#include "S.Bus.h"

#define CPU_HZ 16000000
#define HF_IRQ_TIMER 1

#define HF_IRQ_CYCLES (uint8_t) (CPU_HZ / SBUS_HZ)

volatile extern int IRQ_count_high;
volatile extern int IRQ_count;

void HF_IRQ_setup();

#endif // HF_ISR
