#include <avr/interrupt.h>
#include <Arduino.h>

#define FOSC 16000000UL
#define BAUD 9600UL
#define MYUBRR FOSC/16/BAUD-1

void usart_init();
void spi_init();
void usart_send(const char *msg);
void usart_recieve();
void validate_rx(const char rx[]);
void spi_send(int pwm_value);
void spi_recieve();

const char *status_text[4] = {            // Pointer
        "Geben Sie einen Integer zwischen 0 und 255 ein: ",       // CR besser als LF?
        "FEHLER: Eingabe hat das falsche Format.\n",
        "FEHLER: LED nicht erreichbar.\n",
        "Helligkeit der LED erfolgreich geändert.\n\n"
};

char recieve[4];

uint16_t new_pwm_value = 0;
uint16_t current_pwm_value = 0;

/**
 * pio device monitor -f send_on_enter -f colorize --echo
 */

int main () {

    usart_init();
    spi_init();

    while (1) {

        usart_send(status_text[0]);         // wait for user input
        usart_recieve();
        validate_rx(recieve);
        if(current_pwm_value != new_pwm_value) {
            spi_send(new_pwm_value);
        }
        _delay_ms(2000);

    }

}

void usart_init() {

    UBRR0 = MYUBRR;                             // Set clock
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);      // enable reciever & transmitter

}

void usart_send(const char *msg) {

    for (int i = 0; i < strlen(msg); ++i) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = msg[i];
    }

}

void usart_recieve() {

    UCSR0B |= (1 << RXEN0);
    for (char & i : recieve) {
        while (!(UCSR0A & (1 << RXC0)));
        i = UDR0;
        if(i == 13) { // CR
            break;
        }
    }
    UCSR0B &= ~(1 << RXEN0);

}

void validate_rx(const char rx[]) {

    uint8_t valid = 0;
    new_pwm_value = 0;

    for (int i = 0; i <= 3; i++) {
        if (rx[i] > 47 && rx[i] < 58) {
            new_pwm_value = (10 * new_pwm_value) + (rx[i] - 48);
        } else if ((rx[i] == '\n') || (rx[i] == '\r')) {
            valid = 1;
            break;
        } else {
            new_pwm_value = 256;
            break;
        }
    }

    if(!valid || new_pwm_value > 255) {
        new_pwm_value = current_pwm_value;
        usart_send(status_text[1]);
    }

}

void spi_init() {
    // Set SS, MOSI and SCK output (all others input)
    DDRB = (1 << DDB2) | (1 << DDB3) | (1 << DDB5);
    // Enable SPI, Master, set clock rate fck/16
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

void spi_send(int pwm_value) {
    // Select Slave (SS Low)
    PORTB &= ~(1 << PORTB2);

//    SPDR = 'V';
//    while (!(SPSR & (1 << SPIF)));

    SPDR = (uint8_t) pwm_value;
    while (!(SPSR & (1 << SPIF)));

    current_pwm_value = pwm_value;

    // slave deselect
    PORTB |= (1 << PORTB2);

}

void spi_recieve() {
}