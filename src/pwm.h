#ifndef PWM_H
#define PWM_H

#include <avr/io.h>
#include <stdint.h>

void pwm_init(void);
void pwm_set_duty(uint8_t duty_cycle);

#endif