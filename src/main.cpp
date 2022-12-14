#include <avr/interrupt.h>
#include <Arduino.h>

/* Setup for USART baud calc */
#define FOSC 16000000UL
#define BAUD 9600UL
#define MYUBRR FOSC/16/BAUD-1

void usart_init();
void spi_init();
void usart_send(const char *msg);
void usart_recieve();
void validate_rx(const char rx[]);
void spi_send(int pwm_value);

/* Our outpus msgs */
const char *status_text[4] = {
        "Geben Sie einen Integer zwischen 0 und 255 ein: ",       // CR besser als LF?
        "FEHLER: Eingabe hat das falsche Format.\n",
        "Helligkeit der LED erfolgreich geändert.\n\n"
};

char recieve[4];

uint16_t new_pwm_value, current_pwm_value = 0;

/**
 * @note: pio device monitor -f send_on_enter -f colorize --echo
 */

int main() {

    usart_init();
    spi_init();

    while (1) {

        usart_send(status_text[0]);                            // greetings
        usart_recieve();                                            // wait for user input
        validate_rx(recieve);                                    // validate input
        if (current_pwm_value != new_pwm_value) {
            spi_send(new_pwm_value);                       // send out new pwm
            usart_send(status_text[2]);
        }
        _delay_ms(1000);                                        // delay for good measure

    }

}

/**
 * Initiate USART registers
 */
void usart_init() {

    UBRR0 = MYUBRR;                                                 // Set clock
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);                          // enable reciever & transmitter

}

/**
 * Send messages to console
 * @param msg
 */
void usart_send(const char *msg) {

    for (int i = 0; i < strlen(msg); ++i) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = msg[i];
    }

}

/**
 * Recieve input from console
 */
void usart_recieve() {

    UCSR0B |= (1 << RXEN0);
    for (char &i: recieve) {
        while (!(UCSR0A & (1 << RXC0)));
        i = UDR0;
        if (i == 13) {                                              // CR
            break;
        }
    }
    UCSR0B &= ~(1 << RXEN0);

}

/**
 * Check whether our input value is valid
 * @param rx
 */
void validate_rx(const char rx[]) {

    uint8_t valid = 0;
    new_pwm_value = 0;

    for (int i = 0; i <= 3; i++) {
        if (rx[i] > 47 && rx[i] < 58) {                             // 0 - 9
            new_pwm_value = (10 * new_pwm_value) + (rx[i] - 48);
        } else if ((rx[i] == '\n') || (rx[i] == '\r')) {            // CR / LF
            valid = 1;
            break;
        } else {                                                    // other
            new_pwm_value = 256;
            break;
        }
    }

    if (!valid || new_pwm_value > 255) {
        new_pwm_value = current_pwm_value;
        usart_send(status_text[1]);
    }

}

/**
 * Initiate SPI regsiters
 */
void spi_init() {

    DDRB = (1 << DDB2) | (1 << DDB3) | (1 << DDB5);             // Set SS, MOSI and SCK output (all others input)
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);              // Enable SPI, Master, set clock rate fck/16

}

void spi_send(int pwm_value) {

    PORTB &= ~(1 << PORTB2);                                    // slave select (SS Low)

    /* send a char before our value, fixes 255 madness */
    SPDR = 'V';
    while (!(SPSR & (1 << SPIF)));

    /* send our pwm value */
    SPDR = (uint8_t) pwm_value;
    while (!(SPSR & (1 << SPIF)));

    current_pwm_value = pwm_value;
    PORTB |= (1 << PORTB2);                                     // slave deselect

}