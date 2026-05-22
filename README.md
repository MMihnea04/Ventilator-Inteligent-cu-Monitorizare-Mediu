# Ventilator inteligent cu monitorizare mediu

Acest proiect reprezinta un sistem embedded dezvoltat in limbajul C, la nivel de registri (bare-metal), pentru familia de microcontrolere AVR (testat pe ATmega328P). Sistemul monitorizeaza in timp real temperatura ambientala si nivelul de gaze inflamabile/fum, ajustand automat turatia unui ventilator de evacuare si afisand starea generala pe un ecran LCD si prin indicatoare vizuale (LED-uri).

## Arhitectura Hardware

Componentele fizice utilizate in proiect:
* **Microcontroler:** ATmega328P
* **Senzor de temperatura:** LM35D
* **Senzor de gaz/fum:** MQ-2
* **Control Motor:** Driver L298N + Ventilator DC 12V
* **Interfata Utilizator (UI):** LCD 16x2 cu modul I2C
* **Indicatoare de stare:** 3x LED-uri (Verde, Galben, Rosu)

### Conexiuni (Pinout)

* **Senzori Analogici / Digitali:**
  * MQ-2 Analog (Gaz): `PC0` (ADC0)
  * MQ-2 Digital (Gaz): `PB4`
  * LM35D Analog (Temp): `PC1` (ADC1)
* **Ventilator (Driver L298N):**
  * IN1 (Directie): `PB2`
  * IN2 (Directie): `PB3`
  * Control PWM turatie: Gestionat via timer intern configurat in `pwm.c`
* **LED-uri Stare:**
  * LED Verde: `PD3`
  * LED Galben: `PD4`
  * LED Rosu: `PD5`
* **LCD I2C:**
  * SDA: `PC4` (A4)
  * SCL: `PC5` (A5)

## Structura Codului Sursa

Proiectul este modularizat pentru a separa driverele hardware de logica de aplicatie:

* `main.c` - Contine bucla principala (Super-Loop), logica de decizie, implementarea filtrelor software si masina de stari.
* `adc.c` / `adc.h` - Initializarea si citirea convertorului Analog-Digital (ADC) pentru senzorii LM35 si MQ-2.
* `pwm.c` / `pwm.h` - Configurarea timerelor hardware pentru generarea semnalului PWM necesar controlului turatiei ventilatorului.
* `i2c.c` / `i2c.h` - Implementarea bare-metal a protocolului TWI (Two-Wire Interface) / I2C la 100kHz.
* `lcd_i2c.c` / `lcd_i2c.h` - Driverul pentru controlul display-ului LCD prin intermediul expander-ului I/O PCF8574.

## Logica de Functionare si Solutii Ingineresti

Sistemul abordeaza cateva probleme clasice de hardware prin solutii software avansate:

### 1. Filtrarea Zgomotului Electric (Ground Bounce)
Pornirea motorului DC genereaza un zgomot pe linia de masa (GND), ceea ce cauzeaza citiri analogice eronate ale senzorului LM35D (salturi de 5-7 grade Celsius). Problema este rezolvata in `main.c` prin utilizarea unui **Filtru EMA (Exponential Moving Average)** combinat cu o medie aritmetica (Oversampling):
* Se fac 20 de citiri consecutive la interval de 2ms.
* Formula EMA: `Temperatura_Netezita = (Temperatura_Netezita * 0.50) + (Citire_Noua * 0.50)`.
* Rezultat: Zgomotul indus de motor este absorbit, temperatura raportata ramanand stabila.

### 2. Implementarea Histerezisului
Pentru a preveni oscilatiile on/off ale ventilatorului atunci cand temperatura oscileaza in jurul pragului de comutare, a fost implementata o zona tampon (Histerezis) de `2.0` grade Celsius. 
Sistemul schimba starea (de ex. de la Galben la Verde) doar dupa ce a depasit cu fermitate pragul de oprire, asigurand functionarea cursiva a motorului.

### 3. Masina de Stari (Controlul Termic)
* **Temperatura Optima (< 35 C):** LED Verde, Ventilator Oprit, LCD afiseaza "Stare: Normal".
* **Atentie (> 35 C):** LED Galben, Ventilator la viteza medie (PWM 150), LCD afiseaza "Viteza Medie".
* **Critic (> 40 C):** LED Rosu, Ventilator la viteza maxima (PWM 255), LCD afiseaza "Temperatura MAX!".

### 4. Prioritate de Intrerupere (Alarma Gaz)
Senzorul MQ-2 reprezinta starea de siguranta a sistemului. Indiferent de temperatura raportata de sistem, daca MQ-2 detecteaza gaz (prag analogic brut > 130 sau pinul digital activat la GND):
* Sistemul intra in stadiul de alarma "Override".
* Ventilatorul primeste comanda de viteza maxima (PWM 255) pentru a evacua gazul.
* LED-ul rosu devine intermitent.
* LCD-ul afiseaza mesajul "ALARMA GAZ!".
* Starea revine la normal doar cand gazul este complet evacuat.

## Debugging (Monitorizare UART)

Microcontrolerul transmite in mod continuu date telemetrice catre PC prin intermediul interfetei UART (hardware TX/RX).

**Parametri conectare:**
* Baud Rate: 9600 bps
* Data bits: 8
* Stop bits: 1
* Parity: None

**Exemplu de output pe consola Seriala:**
```text
Temp: 31.5 C | MQ2_Analog: 84 | Pin_Dig: 1 | Alarma: NU
Temp: 32.2 C | MQ2_Analog: 85 | Pin_Dig: 1 | Alarma: NU
Temp: 35.8 C | MQ2_Analog: 160 | Pin_Dig: 0 | Alarma: DA