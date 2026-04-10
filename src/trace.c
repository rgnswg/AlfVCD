/*
 * trace.c — Auto-discovery and VCD signal registration for vcdgen.
 *
 * Probes IO ports 'A'..'F' using AVR_IOCTL_IOPORT_GETSTATE. For each
 * port that the MCU actually has, registers:
 *   - One 8-bit VCD signal for the whole port byte (e.g. "PB")
 *   - Eight 1-bit VCD signals for each pin (e.g. "PB0".."PB7")
 *
 * This is intentionally structured so adding new signal kinds (timers,
 * UART lines, etc.) is easy: just follow the same pattern.
 */

#include "trace.h"
#include "avr_ioport.h"
#include "sim_io.h"
#include <stdio.h>
#include <string.h>

/* Ports to probe, in order. Extend here for other MCUs. */
static const char PORTS[] = {'A', 'B', 'C', 'D', 'E', 'F'};
#define NUM_PORTS ((int)(sizeof(PORTS) / sizeof(PORTS[0])))

int trace_setup(avr_t *avr, avr_vcd_t *vcd, int verbose) {
  int total = 0;

  for (int pi = 0; pi < NUM_PORTS; pi++) {
    char port = PORTS[pi];

    /*
     * Probe: avr_ioctl returns 0 if the port exists.
     * We use GETSTATE just as a presence check; the struct
     * will be filled with the current port/ddr/pin values.
     */
    avr_ioport_state_t state;
    if (avr_ioctl(avr, AVR_IOCTL_IOPORT_GETSTATE(port), &state) != 0)
      continue; /* Port doesn't exist on this MCU */

    if (verbose)
      printf("[trace] Found PORT%c (port=0x%02x ddr=0x%02x pin=0x%02x)\n", port,
             state.port, state.ddr, state.pin);

    /* --- 8-bit bus signal for the whole port --- */
    avr_irq_t *port_irq =
        avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(port), IOPORT_IRQ_PIN_ALL);
    if (port_irq) {
      char name[4]; /* "PA\0" */
      snprintf(name, sizeof(name), "P%c", port);
      if (avr_vcd_add_signal(vcd, port_irq, 8, name) == 0) {
        total++;
        if (verbose)
          printf("[trace]   + signal %-6s (8-bit bus)\n", name);
      } else {
        fprintf(stderr, "[trace] warning: could not add signal %s\n", name);
      }
    }

    /* --- 1-bit signal per pin --- */
    for (int bit = 0; bit < 8; bit++) {
      avr_irq_t *pin_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(port),
                                         IOPORT_IRQ_PIN0 + bit);
      if (!pin_irq)
        continue;

      char name[6]; /* "PB7\0" */
      snprintf(name, sizeof(name), "P%c%d", port, bit);
      if (avr_vcd_add_signal(vcd, pin_irq, 1, name) == 0) {
        total++;
        if (verbose)
          printf("[trace]   + signal %-6s (bit %d)\n", name, bit);
      } else {
        fprintf(stderr, "[trace] warning: could not add signal %s\n", name);
      }
    }
  }

  return total;
}
