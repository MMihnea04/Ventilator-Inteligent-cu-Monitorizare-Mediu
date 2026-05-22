#include "lcd_i2c.h"
#include "i2c.h"
#include <util/delay.h>

// Adresa 0x27 shiftata cu 1 bit spre stanga pentru scriere
#define LCD_ADDR 0x4E 

void lcd_write_nibble(uint8_t nibble, uint8_t rs) {
    // Nibble-ul superior + RS + Lumina de fundal (0x08)
    uint8_t data = (nibble & 0xF0) | rs | 0x08; 
    
    i2c_start();
    i2c_write(LCD_ADDR);
    
    // Puls pe pinul Enable (EN)
    i2c_write(data | 0x04); // EN = 1
    _delay_us(1);
    i2c_write(data & ~0x04); // EN = 0
    _delay_us(50);
    
    i2c_stop();
}

void lcd_send_cmd(uint8_t cmd) {
    lcd_write_nibble(cmd & 0xF0, 0);        // RS = 0 pentru comenzi
    lcd_write_nibble((cmd << 4) & 0xF0, 0);
}

void lcd_send_char(char data) {
    lcd_write_nibble(data & 0xF0, 1);       // RS = 1 pentru caractere
    lcd_write_nibble((data << 4) & 0xF0, 1);
}

void lcd_init(void) {
    i2c_init();
    _delay_ms(50); // Asteptam sa porneasca modulul
    
    // Secventa de initializare pentru modul 4-biti (Datasheet HD44780)
    lcd_write_nibble(0x30, 0); _delay_ms(5);
    lcd_write_nibble(0x30, 0); _delay_us(150);
    lcd_write_nibble(0x30, 0); _delay_us(150);
    lcd_write_nibble(0x20, 0); _delay_us(150); // Trecem in modul 4-biti
    
    lcd_send_cmd(0x28); // 2 linii, font 5x8
    lcd_send_cmd(0x0C); // Porneste ecranul, ascunde cursorul
    lcd_send_cmd(0x06); // Scriere de la stanga la dreapta
    lcd_clear();
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);
    _delay_ms(2); // Clear dureaza mai mult
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t pos = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(pos);
}

void lcd_print(const char *str) {
    while (*str) {
        lcd_send_char(*str++);
    }
}