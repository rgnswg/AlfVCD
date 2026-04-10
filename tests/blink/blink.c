/*
 * blink.c — Test AVR firmware for vcdgen.
 *
 * ATmega328p @ 16 MHz.
 * Toggles PORTB bit 0 (PB0) a fixed number of times using a busy-wait
 * loop, then executes SLEEP to halt the simulator cleanly (cpu_Done).
 *
 * Expected VCD output: PB0 alternates 0→1→0→1... then stays constant.
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

/*
 * Delay in real-time milliseconds (relies on F_CPU and __delay_ms).
 * At 16 MHz this is accurate; simavr will run them as cycles.
 */
#define BLINK_DELAY_MS 1

#define BLINK_COUNT 6 /* 3 full on/off cycles → easily visible in GTKWave */

int main(void) {
  /* PORTB all outputs */
  DDRB = 0xFF;
  PORTB = 0x00;

  for (int i = 0; i < BLINK_COUNT; i++) {
    PORTB ^= (1 << PB0); /* Toggle PB0 */
    _delay_ms(BLINK_DELAY_MS);
  }

  /* Also toggle PB1 once so we have two distinct pins active */
  PORTB |= (1 << PB1);
  _delay_ms(BLINK_DELAY_MS);
  PORTB &= ~(1 << PB1);

  /* Halt the simulator: SLEEP with SE bit set and interrupts disabled
   * produces cpu_Done in simavr when no interrupt can wake the core. */
  cli();
  sleep_enable();
  sleep_cpu();

  /* Never reached */
  return 0;
}
