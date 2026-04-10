/*
 * run.c — Simulation loop for vcdgen.
 *
 * Wraps avr_run() with:
 *   - Cycle-based timeout     (usec_limit > 0)
 *   - Indefinite run until firmware halts or crashes (usec_limit == 0)
 */

#include "run.h"
#include <stdio.h>

run_result_t run_sim(avr_t *avr, uint64_t usec_limit) {
  /*
   * Convert microsecond limit to AVR cycles.
   * cycle_limit == 0 means "run indefinitely".
   */
  uint64_t cycle_limit = 0;
  if (usec_limit > 0)
    cycle_limit = (uint64_t)usec_limit * avr->frequency / 1000000ULL;

  for (;;) {
    int state = avr_run(avr);

    if (state == cpu_Done)
      return RUN_DONE;

    if (state == cpu_Crashed)
      return RUN_CRASHED;

    /* Check timeout (avr->cycle monotonically increases) */
    if (cycle_limit && avr->cycle >= cycle_limit)
      return RUN_TIMEOUT;
  }
}
