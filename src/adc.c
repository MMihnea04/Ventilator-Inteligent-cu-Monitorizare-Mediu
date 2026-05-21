#include "adc.h"

void adc_init(void) {
    // selectam refer AVCC(5V) cu condensator pe AREF
    ADMUX = (1 << REFS0);
    
    // setam prescaler-ul la 128 (16MHz / 128 = 125 kHz)
    // pornim modulul ADC (ADEN)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel) {
    // ne asiguram ca nr canalului e intre 0 si 7
    channel &= 0x07; 
    
    // cratam bitii de multiplexare vechi din ADMUX si introducem canalul nou
    ADMUX = (ADMUX & 0xF8) | channel;
    
    // incepem conversia
    ADCSRA |= (1 << ADSC);
    
    // asteptam pana cand bitul ADSC redevine 0
    while (ADCSRA & (1 << ADSC));
    
    // return val pe 10 biti
    return ADC;
}