# vcdgen Makefile
#
# Detects simavr source location and builds against libsimavr.a.
# Requires: clang or gcc, libelf.

# --- Configuration -----------------------------------------------------------

SIMAVR_ROOT ?= $(HOME)/simavr/simavr
SIMAVR_OBJDIR = $(shell ls -d $(SIMAVR_ROOT)/obj-* 2>/dev/null | head -1)

SIMAVR_INC   = -I$(SIMAVR_ROOT)/sim -I$(SIMAVR_ROOT)/sim/avr
SIMAVR_LIB   = $(SIMAVR_OBJDIR)/libsimavr.a

# --- Compiler settings -------------------------------------------------------

CC      = cc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -Wshadow $(SIMAVR_INC)
LDFLAGS = $(SIMAVR_LIB)

# On macOS, add Homebrew includes for any optional headers
ifeq ($(shell uname -s),Darwin)
    HOMEBREW_PREFIX ?= $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
    CFLAGS += -I$(HOMEBREW_PREFIX)/include
endif

# Optional: enable ELF firmware loading (requires libelf)
# Uncomment the two lines below if simavr was built with libelf support:
# CFLAGS  += -DVCDGEN_ELF
# LDFLAGS += -lelf -L$(HOMEBREW_PREFIX)/lib

# --- Sources -----------------------------------------------------------------

SRCDIR  = src
SRCS    = $(SRCDIR)/main.c $(SRCDIR)/trace.c $(SRCDIR)/run.c
OBJS    = $(SRCS:.c=.o)
TARGET  = alfvcd

# --- Rules -------------------------------------------------------------------

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C tests/blink clean 2>/dev/null || true

test: all
	$(MAKE) -C tests/blink
	@bash tests/run_tests.sh

# Convenience: show detected paths
info:
	@echo "SIMAVR_ROOT   = $(SIMAVR_ROOT)"
	@echo "SIMAVR_OBJDIR = $(SIMAVR_OBJDIR)"
	@echo "SIMAVR_LIB    = $(SIMAVR_LIB)"
