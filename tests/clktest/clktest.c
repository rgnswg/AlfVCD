/*
 * clktest.c — Verificación de frecuencia de simulación
 *
 * Genera 10 pulsos en PD0 usando NOPs.
 * A 16 MHz cada NOP = 1 ciclo = 62.5 ns.
 *
 * Estructura de cada pulso:
 *   HIGH: SET PD0   (1 ciclo = 62.5 ns)
 *         NOP       (1 ciclo = 62.5 ns)
 *         NOP       (1 ciclo = 62.5 ns)
 *   → duracion HIGH = instruccion SET + 2 NOPs = ~3 ciclos totales
 *     pero medido desde flanco subida hasta bajada = 2 NOPs = 125 ns
 *
 * En la practica, cada half-period (HIGH o LOW) = 2 NOPs = 125 ns
 * → periodo completo = 250 ns → f = 4 MHz efectiva
 *
 * Para verificar 16 MHz: medi la duracion de 1 NOP.
 * Si 1 NOP = 62.5 ns en GTKWave → frecuencia = 16 MHz ✓
 *
 * Compilar:  make
 * Simular:   ../../alfvcd -m atmega328p -f 16000000 -o clktest.vcd clktest.hex
 * Ver:       gtkwave clktest.vcd
 *
 * En GTKWave busca la señal PD0:
 *   - Cada HIGH deberia durar 3 ciclos = 187.5 ns
 *   - Cada LOW deberia durar 3 ciclos  = 187.5 ns
 *   - Periodo total = 375 ns
 *   - 10 pulsos → duracion total = 10 * 375 ns = 3750 ns = 3.75 us
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/*
 * Macro: un pulso completo en PD0.
 * HIGH por 3 ciclos (SET + 2 NOPs), LOW por 3 ciclos (CLR + 2 NOPs).
 * Nota: SBI/CBI toman 2 ciclos cada uno en ATmega328p.
 *       NOP toma 1 ciclo.
 *
 * HIGH: SBI (2 ciclos) + NOP + NOP = 4 ciclos = 250 ns
 * LOW:  CBI (2 ciclos) + NOP + NOP = 4 ciclos = 250 ns
 * Periodo = 500 ns → medible facilmente en GTKWave
 *
 * Para confirmar 16 MHz: 1 NOP debe verse como 62.5 ns.
 */
#define PULSE() do {            \
    PORTD |=  (1 << PD0);      \
    __asm__ __volatile__("nop"); \
    __asm__ __volatile__("nop"); \
    PORTD &= ~(1 << PD0);      \
    __asm__ __volatile__("nop"); \
    __asm__ __volatile__("nop"); \
} while(0)

int main(void) {
    /* Configurar PD0 como salida */
    DDRD |= (1 << PD0);
    PORTD = 0x00;

    /* 10 pulsos */
    PULSE();
    PULSE();
    PULSE();
    PULSE();
    PULSE();
    PULSE();
    PULSE();
    PULSE();
    PULSE();
    PULSE();

    /* Halt limpio */
    cli();
    sleep_enable();
    sleep_cpu();

    return 0;
}
