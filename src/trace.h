/*
 * trace.h — Auto-discovery and VCD signal registration for vcdgen.
 *
 * Probes which IO ports exist on the AVR instance and registers
 * each port (8-bit bus) and each individual pin as VCD signals.
 */

#ifndef VCDGEN_TRACE_H
#define VCDGEN_TRACE_H

#include "sim_avr.h"
#include "sim_vcd_file.h"

/*
 * trace_setup() — Discover available IO ports and register VCD signals.
 *
 * Probes ports 'A' through 'F'. For each port that exists on the MCU,
 * registers:
 *   - One 8-bit bus signal:  "PA", "PB", ...
 *   - Eight 1-bit signals:   "PA0".."PA7", "PB0".."PB7", ...
 *
 * Must be called after avr_init() + avr_load_firmware() and before
 * avr_vcd_start().
 *
 * Returns the number of signals registered, or -1 on error.
 */
int trace_setup(avr_t *avr, avr_vcd_t *vcd, int verbose);

#endif /* VCDGEN_TRACE_H */
