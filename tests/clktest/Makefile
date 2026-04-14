# tests/clktest/Makefile
#
# Firmware de verificacion de frecuencia de reloj.
# Genera 10 pulsos en PD0 usando NOPs para que en GTKWave
# se pueda medir exactamente cuanto dura cada ciclo.
#
# A 16 MHz: 1 ciclo = 62.5 ns
#   SBI/CBI = 2 ciclos = 125 ns
#   NOP     = 1 ciclo  = 62.5 ns
#
# Estructura de cada pulso (HIGH = SBI + 2 NOPs, LOW = CBI + 2 NOPs):
#   HIGH = 4 ciclos = 250 ns
#   LOW  = 4 ciclos = 250 ns
#   Periodo = 500 ns (2 MHz efectivo en la señal)
#
# Si en GTKWave cada HIGH mide 250 ns y cada LOW mide 250 ns → 16 MHz ✓

MCU     = atmega328p
F_CPU   = 16000000UL
CC      = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -O0 -Wall -std=c11 -fno-optimize-sibling-calls
TARGET  = clktest

.PHONY: all clean sim

all: $(TARGET).hex

$(TARGET).elf: $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Simular y generar VCD
sim: $(TARGET).hex
	../../alfvcd -m $(MCU) -f 16000000 -o $(TARGET).vcd $(TARGET).hex
	@echo ""
	@echo "=== Verificacion de frecuencia @ 16 MHz ==="
	@echo "Pulsos esperados en PD0:"
	@echo "  HIGH = 4 ciclos = 250 ns (SBI + 2xNOP)"
	@echo "  LOW  = 4 ciclos = 250 ns (CBI + 2xNOP)"
	@echo "  Periodo = 500 ns"
	@echo "  10 pulsos → duracion total aprox = 5000 ns = 5 us"
	@echo ""
	@echo "Abre $(TARGET).vcd con GTKWave y agrega señal PD0."
	@echo "Si cada HIGH/LOW mide ~250 ns → frecuencia = 16 MHz ✓"

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).vcd
