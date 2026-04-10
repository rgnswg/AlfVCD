/*
 * run.h — Simulation loop for vcdgen.
 *
 * Wraps avr_run() with cycle-based timeout and halt detection.
 */

#ifndef VCDGEN_RUN_H
#define VCDGEN_RUN_H

#include "sim_avr.h"
#include <stdint.h>

typedef enum {
  RUN_TIMEOUT = 0, /* Stopped because usec_limit was reached */
  RUN_DONE,        /* Firmware executed SLEEP/HALT (cpu_Done) */
  RUN_CRASHED,     /* Watchdog fired or bad opcode (cpu_Crashed) */
} run_result_t;

/*
 * run_sim() — Execute the AVR simulation loop.
 *
 * If usec_limit == 0 runs indefinitely until the firmware halts or crashes.
 * Otherwise runs until avr->cycle >= (usec_limit * avr->frequency / 1e6).
 *
 * Returns the reason for stopping.
 */
run_result_t run_sim(avr_t *avr, uint64_t usec_limit);

#endif /* VCDGEN_RUN_H */
