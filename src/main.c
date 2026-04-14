/*
 * main.c — vcdgen entry point.
 *
 * Parses CLI arguments, loads a firmware file (.hex or .elf), initialises
 * simavr, registers VCD signals for all GPIO ports, runs the simulation,
 * and writes a .vcd file ready for GTKWave.
 *
 * Usage:
 *   vcdgen [OPTIONS] <firmware.hex|firmware.elf>
 *
 * Options:
 *   -m, --mcu <name>      MCU type (required for .hex, auto from .elf)
 *   -f, --freq <hz>       Clock frequency (required for .hex, auto from .elf)
 *   -o, --output <file>   VCD output file (default: trace.vcd)
 *   -t, --time <usec>     Simulation time in microseconds (default: 1000000)
 *       --indefinite      Run until firmware halts (cpu_Done / cpu_Crashed)
 *   -v, --verbose         Print discovered ports and registered signals
 *   -h, --help            Show this help and exit
 */

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_avr.h"
#include "sim_hex.h"
#include "sim_vcd_file.h"

/* ELF support is optional — only compiled in if libelf is present */
#ifdef VCDGEN_ELF
#include "sim_elf.h"
#endif

#include "run.h"
#include "trace.h"

/* ------------------------------------------------------------------ */
/* Defaults                                                             */
/* ------------------------------------------------------------------ */

#define DEFAULT_OUTPUT "trace.vcd"
#define DEFAULT_USEC 1000000ULL /* 1 second of simulated time   */
#define DEFAULT_FREQ 16000000UL
#define VCD_PERIOD_USEC 10 /* VCD flush granularity in µs  */

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static int g_verbose = 0;

static void quiet_logger(struct avr_t *avr, const int level, const char *fmt,
                         va_list ap) {
  (void)avr;
  if (!g_verbose && level > LOG_ERROR)
    return;
  vfprintf(stderr, fmt, ap);
}

static void usage(const char *prog) {
  fprintf(
      stderr,
      "Usage: %s [OPTIONS] <firmware.hex|firmware.elf>\n"
      "\n"
      "Options:\n"
      "  -m, --mcu <name>      MCU type (required for .hex)\n"
      "  -f, --freq <hz>       Clock frequency in Hz (required for .hex, "
      "default: %lu)\n"
      "  -o, --output <file>   VCD output file (default: %s)\n"
      "  -t, --time <usec>     Simulation time in microseconds (default: "
      "%llu)\n"
      "      --indefinite      Run until firmware halts or crashes\n"
      "  -v, --verbose         Print discovered ports and registered signals\n"
      "  -h, --help            Show this help and exit\n",
      prog, DEFAULT_FREQ, DEFAULT_OUTPUT, DEFAULT_USEC);
}

/* Returns pointer to file extension (including '.'), or "" if none. */
static const char *file_ext(const char *path) {
  const char *dot = strrchr(path, '.');
  return dot ? dot : "";
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
  /* --- Option state --- */
  const char *opt_mcu = NULL;
  unsigned long opt_freq = 0; /* 0 = use ELF or default */
  const char *opt_output = DEFAULT_OUTPUT;
  uint64_t opt_usec = DEFAULT_USEC;
  int opt_indefinite = 0;

  static const struct option long_opts[] = {
      {"mcu", required_argument, NULL, 'm'},
      {"freq", required_argument, NULL, 'f'},
      {"output", required_argument, NULL, 'o'},
      {"time", required_argument, NULL, 't'},
      {"indefinite", no_argument, NULL, 'I'},
      {"verbose", no_argument, NULL, 'v'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "m:f:o:t:vh", long_opts, NULL)) != -1) {
    switch (c) {
    case 'm':
      opt_mcu = optarg;
      break;
    case 'f':
      opt_freq = strtoul(optarg, NULL, 10);
      break;
    case 'o':
      opt_output = optarg;
      break;
    case 't':
      opt_usec = strtoull(optarg, NULL, 10);
      break;
    case 'I':
      opt_indefinite = 1;
      break;
    case 'v':
      g_verbose = 1;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    default:
      usage(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Error: no firmware file specified.\n\n");
    usage(argv[0]);
    return 1;
  }
  const char *firmware_path = argv[optind];

  /* Install quiet logger before any simavr calls */
  avr_global_logger_set(quiet_logger);

  /* ---------------------------------------------------------------- */
  /* Load firmware                                                      */
  /* ---------------------------------------------------------------- */

  const char *ext = file_ext(firmware_path);
  const char *mcu_name = NULL;
  uint32_t freq = 0;

  /* We keep the flash data and size for HEX loading */
  uint8_t *hex_data = NULL;
  uint32_t hex_size = 0;
  uint32_t hex_start = 0;

  int is_hex = (strcasecmp(ext, ".hex") == 0);
  int is_elf = (strcasecmp(ext, ".elf") == 0);

  if (!is_hex && !is_elf) {
    fprintf(stderr,
            "Error: unknown firmware extension '%s'. Use .hex or .elf.\n", ext);
    return 1;
  }

  if (is_hex) {
    /* HEX: MCU and freq must come from CLI */
    if (!opt_mcu) {
      fprintf(stderr,
              "Error: --mcu is required for .hex firmware.\n"
              "  Example: vcdgen -m atmega328p -f 16000000 firmware.hex\n");
      return 1;
    }
    mcu_name = opt_mcu;
    freq = (uint32_t)(opt_freq ? opt_freq : DEFAULT_FREQ);

    hex_data = read_ihex_file(firmware_path, &hex_size, &hex_start);
    if (!hex_data) {
      fprintf(stderr, "Error: could not read HEX file '%s'\n", firmware_path);
      return 1;
    }
    if (g_verbose)
      printf("[vcdgen] HEX loaded: %u bytes at 0x%04x\n", hex_size, hex_start);
  }

#ifdef VCDGEN_ELF
  elf_firmware_t fw;
  memset(&fw, 0, sizeof(fw));

  if (is_elf) {
    if (elf_read_firmware(firmware_path, &fw) != 0) {
      fprintf(stderr, "Error: could not read ELF file '%s'\n", firmware_path);
      return 1;
    }
    mcu_name = opt_mcu ? opt_mcu : (fw.mmcu[0] ? fw.mmcu : NULL);
    if (!mcu_name) {
      fprintf(stderr, "Error: MCU not found in ELF. Specify with --mcu.\n");
      return 1;
    }
    freq = (uint32_t)(opt_freq ? opt_freq
                               : (fw.frequency ? fw.frequency : DEFAULT_FREQ));
  }
#else
  if (is_elf) {
    fprintf(stderr,
            "Error: ELF support was not compiled in (libelf missing).\n"
            "Use a .hex file instead. Compile with avr-objcopy to convert:\n"
            "  avr-objcopy -O ihex firmware.elf firmware.hex\n");
    return 1;
  }
#endif

  if (g_verbose)
    printf("[vcdgen] firmware=%s mcu=%s freq=%u\n", firmware_path, mcu_name,
           freq);

  /* ---------------------------------------------------------------- */
  /* Initialise AVR                                                    */
  /* ---------------------------------------------------------------- */

  avr_t *avr = avr_make_mcu_by_name(mcu_name);
  if (!avr) {
    fprintf(stderr, "Error: unknown MCU '%s'\n", mcu_name);
    free(hex_data);
    return 1;
  }
  avr_init(avr);
  avr->frequency = freq; /* Must be set AFTER avr_init(): avr_init() hardcodes frequency=1MHz */

  if (is_hex) {
    avr_loadcode(avr, hex_data, hex_size, hex_start);
    free(hex_data);
    hex_data = NULL;
  }
#ifdef VCDGEN_ELF
  else {
    avr_load_firmware(avr, &fw);
  }
#endif

  /* ---------------------------------------------------------------- */
  /* VCD initialisation                                                */
  /* ---------------------------------------------------------------- */

  avr_vcd_t vcd;
  if (avr_vcd_init(avr, opt_output, &vcd, VCD_PERIOD_USEC) != 0) {
    fprintf(stderr, "Error: could not initialise VCD file '%s'\n", opt_output);
    avr_terminate(avr);
    return 1;
  }

  int nsignals = trace_setup(avr, &vcd, g_verbose);
  if (nsignals <= 0) {
    fprintf(stderr,
            "Warning: no GPIO signals found for MCU '%s'. VCD will be empty.\n",
            mcu_name);
  } else if (g_verbose) {
    printf("[vcdgen] Registered %d VCD signal(s)\n", nsignals);
  }

  avr_vcd_start(&vcd);

  /* ---------------------------------------------------------------- */
  /* Simulation                                                        */
  /* ---------------------------------------------------------------- */

  uint64_t usec_limit = opt_indefinite ? 0 : opt_usec;

  if (g_verbose) {
    if (usec_limit)
      printf("[vcdgen] Running for %llu µs...\n", usec_limit);
    else
      printf("[vcdgen] Running indefinitely...\n");
  }

  run_result_t result = run_sim(avr, usec_limit);

  /* ---------------------------------------------------------------- */
  /* Cleanup                                                           */
  /* ---------------------------------------------------------------- */

  avr_vcd_stop(&vcd);
  avr_vcd_close(&vcd);
  avr_terminate(avr);

  const char *reason = (result == RUN_DONE)      ? "firmware halted"
                       : (result == RUN_CRASHED) ? "firmware crashed"
                                                 : "time limit reached";

  printf("Simulation stopped: %s\n", reason);
  printf("VCD written to: %s\n", opt_output);

  return (result == RUN_CRASHED) ? 2 : 0;
}
