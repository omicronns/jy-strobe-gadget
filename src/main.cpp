#include <Arduino.h>

#define PIN_LED 1

void start_timer(void)
{		
  NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER2->TASKS_CLEAR = 1;
	NRF_TIMER2->PRESCALER = 7;
	NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
	NRF_TIMER2->CC[0] = 1000;
	NRF_TIMER2->CC[1] = 500;
  NRF_TIMER2->SHORTS = 1 << 0;

  // Enable interrupt on Timer 2, both for CC[0] and CC[1] compare match events
	NRF_TIMER2->INTENSET =
    (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos) |
    (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
  NVIC_EnableIRQ(TIMER2_IRQn);
		
  NRF_TIMER2->TASKS_START = 1;               // Start TIMER2
}

void timer_freq(int freq)
{
  int cnt = 125000 / freq;
	NRF_TIMER2->CC[0] = cnt;
	NRF_TIMER2->CC[1] = cnt / 2;
}

void timer_rpm(int rpm)
{
  int cnt = 125000 * 60 / rpm;
	NRF_TIMER2->CC[0] = cnt;
	NRF_TIMER2->CC[1] = cnt / 2;
}

extern "C" {

void TIMER2_IRQHandler(void)
{
	if ((NRF_TIMER2->EVENTS_COMPARE[0] != 0) && ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE0_Msk) != 0))
  {
		NRF_TIMER2->EVENTS_COMPARE[0] = 0;           //Clear compare register 0 event	
		NRF_GPIO->OUTSET = 1 << PIN_LED;
  }
	
	if ((NRF_TIMER2->EVENTS_COMPARE[1] != 0) && ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE1_Msk) != 0))
  {
		NRF_TIMER2->EVENTS_COMPARE[1] = 0;           //Clear compare register 1 event
		NRF_GPIO->OUTCLR = 1 << PIN_LED;
  }
}

}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  start_timer();
  timer_freq(1000);
}

void loop() {
}
