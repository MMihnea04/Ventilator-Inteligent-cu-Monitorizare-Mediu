#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "adc.h"
#include "pwm.h"
#include "lcd_i2c.h"

// pini
#define MQ2_DO_PIN      PB4
#define LED_GREEN_PIN   PD3
#define LED_YELLOW_PIN  PD4
#define LED_RED_PIN     PD5
#define FAN_IN1_PIN     PB2
#define FAN_IN2_PIN     PB3

// prag temp(poate varia deoarece celelalte componente pot influenta citirea)
#define TEMP_LOW        33.0  
#define TEMP_HIGH       38.0  
#define HYSTERESIS      2.0   

// prag Gaz calibrat dupa log-ul real
#define GAS_ANALOG_THRESHOLD 130 

#define PWM_OFF         0
#define PWM_MEDIUM      150
#define PWM_MAX         255

// functii UART pt debug
void uart_init(void) {
    UBRR0H = (103 >> 8);
    UBRR0L = 103;
    UCSR0B = (1 << TXEN0); 
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 
}

void uart_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0))); 
    UDR0 = data;
}

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

void gpio_init(void) {
    DDRD |= (1 << LED_GREEN_PIN) | (1 << LED_YELLOW_PIN) | (1 << LED_RED_PIN);
    DDRB |= (1 << FAN_IN1_PIN) | (1 << FAN_IN2_PIN);
    DDRB &= ~(1 << MQ2_DO_PIN);
    PORTD &= ~((1 << LED_GREEN_PIN) | (1 << LED_YELLOW_PIN) | (1 << LED_RED_PIN));
    PORTB |= (1 << FAN_IN1_PIN);
    PORTB &= ~(1 << FAN_IN2_PIN);
}

int main(void) {
    adc_init();
    pwm_init();
    gpio_init();
    uart_init(); 
    lcd_init(); // init lcd
    
    uint8_t current_mode = 0; 
    char debug_buffer[80]; 
    char lcd_buffer[16]; // pt afisare lcd
    
    uart_print("\r\n--- SISTEM INTELIGENT PREZENTARE (PRAGURI OPTIMIZATE) ---\r\n");
    
    // mesaj de pornire lcd
    lcd_set_cursor(0, 0);
    lcd_print("Sistem Activat!");
    _delay_ms(1500);
    lcd_clear();
    
    static float smoothed_temp = -1.0; 
    
    while (1) {
        uint32_t sum_lm35 = 0;
        uint32_t sum_mq2 = 0;
        
        for (uint8_t i = 0; i < 20; i++) {
            sum_lm35 += adc_read(1); 
            sum_mq2  += adc_read(0); 
            _delay_ms(2);            
        }
        
        uint16_t adc_lm35 = sum_lm35 / 20;
        uint16_t adc_mq2  = sum_mq2 / 20;
        
        float voltage_mv = ((uint32_t)adc_lm35 * 5000) / 1024.0;
        float raw_temperature = voltage_mv / 10.0;
        
        // filtru pentru zgomotul motorului
        if (smoothed_temp < 0) {
            smoothed_temp = raw_temperature; 
        } else {
            smoothed_temp = (smoothed_temp * 0.50) + (raw_temperature * 0.50);
        }

        // citire digitala si evaluare gaz
        uint8_t digital_mq2_pin = (PINB & (1 << MQ2_DO_PIN)) ? 1 : 0;
        uint8_t gas_detected_digital = !digital_mq2_pin; 
        
        // logica centralizata de alarmacdaca sare de 130 SAU pinul digital se activeaza
        uint8_t alarm_active = (gas_detected_digital || adc_mq2 > GAS_ANALOG_THRESHOLD);

        int temp_int = (int)smoothed_temp;
        int temp_dec = (int)(smoothed_temp * 10) % 10;
        
        sprintf(debug_buffer, "Temp: %d.%d C | MQ2_Analog: %u | Pin_Dig: %d | Alarma: %s\r\n", 
                temp_int, temp_dec, adc_mq2, digital_mq2_pin, alarm_active ? "DA" : "NU");
        uart_print(debug_buffer);

        // update lcd
        lcd_set_cursor(0, 0);
        sprintf(lcd_buffer, "Temp: %d.%d\xDF""C   ", temp_int, temp_dec); // \xDF e simbolul de grade
        lcd_print(lcd_buffer);
        
        lcd_set_cursor(1, 0); // linia a 2-a

        PORTD &= ~((1 << LED_GREEN_PIN) | (1 << LED_YELLOW_PIN) | (1 << LED_RED_PIN));

        // logica prioritara actiune
        if (alarm_active) { 
            lcd_print("ALARMA GAZ!     ");
            pwm_set_duty(PWM_MAX);
            PORTD |= (1 << LED_RED_PIN);
            _delay_ms(250);
            PORTD &= ~(1 << LED_RED_PIN);
            _delay_ms(250);
        } 
        else {
            // functionare normala temp
            if (current_mode == 0) { 
                if (smoothed_temp > TEMP_LOW + HYSTERESIS) current_mode = 1;
            } 
            else if (current_mode == 1) { 
                if (smoothed_temp < TEMP_LOW - HYSTERESIS) current_mode = 0;
                else if (smoothed_temp > TEMP_HIGH + HYSTERESIS) current_mode = 2;
            } 
            else if (current_mode == 2) { 
                if (smoothed_temp < TEMP_HIGH - HYSTERESIS) current_mode = 1;
            }

            if (current_mode == 0) {
                lcd_print("Stare: Normal   ");
                pwm_set_duty(PWM_OFF);
                PORTD |= (1 << LED_GREEN_PIN); 
            } 
            else if (current_mode == 1) {
                lcd_print("Viteza Medie    ");
                pwm_set_duty(PWM_MEDIUM);
                PORTD |= (1 << LED_YELLOW_PIN); 
            } 
            else {
                lcd_print("Temperatura MAX!");
                pwm_set_duty(PWM_MAX);
                PORTD |= (1 << LED_RED_PIN); 
            }
            
            _delay_ms(500); 
        }
    }
    return 0;
}