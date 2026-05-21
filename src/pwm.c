#include "pwm.h"

void pwm_init(void) {
    // setam pinul PB1 ca iesire
    DDRB |= (1 << PB1);
    
    // config timer 1 in modul Fast PWM, 8-bit
    // clear OC1A pe Compare Match (non-inverting mode)
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    
    // Setam Prescaler la 64
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
    
    // init ventilatorul pe oprit
    OCR1A = 0; 
}

// duty_cycle trebuie sa fie intre 0 si 255
void pwm_set_duty(uint8_t duty_cycle) {
    OCR1A = duty_cycle;
}