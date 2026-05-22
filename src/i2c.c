#include <avr/io.h>
#include "i2c.h"

void i2c_init(void) {
    TWSR = 0x00; // Prescaler 1
    // Seteaza frecventa I2C la 100kHz (Presupunand F_CPU = 16MHz)
    TWBR = 72;   
    TWCR = (1 << TWEN); // Activeaza modulul TWI
}

void i2c_start(void) {
    // Trimite semnalul de START
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))); // Asteapta terminarea actiunii
}

void i2c_write(uint8_t data) {
    // Incarca datele si trimite-le
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))); // Asteapta terminarea actiunii
}

void i2c_stop(void) {
    // Trimite semnalul de STOP
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}