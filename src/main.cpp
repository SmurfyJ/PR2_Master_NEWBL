#include <avr/interrupt.h>
#include <Arduino.h>

#define FOSC 16000000UL
#define BAUD 9600UL
#define MYUBRR FOSC/16/BAUD-1

void usart_init();
void usart_send(const char *msg);
void usart_recieve();
void validate_rx(const char rx[]);

const char *status_text[6] = {            // Pointer
        "\rGeben Sie einen Integer zwischen 0 und 255 ein: ",       // CR besser als LF?
        "\rFEHLER: Eingabe hat das falsche Format.\t",
        "FEHLER: LED nicht erreichbar.",
        "Helligkeit der LED erfolgreich geändert.\n",
        "CARRIAGE RETURN OR LINEFEED\n",
        "0 < x < 10"
};

char recieve[16];

int new_pwm_value = 0;
int current_pwm_value = 0;
int flag = 0;

/**
 * pio device monitor -f send_on_enter -f colorize --echo
 */

int main () {

    usart_init();

    while (1) {

        usart_send(status_text[0]);         // wait for user input
        usart_recieve();
        validate_rx(recieve);

        if(current_pwm_value != new_pwm_value) {
            //kockoff;
        }

        _delay_ms(2000);

    }

}

void usart_init() {

    UBRR0 = MYUBRR;                             // Set clock
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);      // enable reciever & transmitter

}

void usart_recieve() {

    UCSR0B |= (1 << RXEN0);
    for (char & i : recieve) {
        while (!(UCSR0A & (1 << RXC0)));
        i = UDR0;
        if(i == '\n') {
            break;
        }
    }
    UCSR0B &= ~(1 << RXEN0);

}

void usart_send(const char *msg) {

    for (int i = 0; i < strlen(msg); ++i) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = msg[i];
    }

}

void validate_rx(const char rx[]) {

    new_pwm_value = 0;

    for (int i = 0; i <= sizeof rx; i++) {
        if (rx[i] < 58 && rx[i] > 47) {
            new_pwm_value = 10 * new_pwm_value + (rx[i] - 48);
        } else if (rx[i] == '\n' || rx[i] == '\r') {
            break;
        } else {
            new_pwm_value = -1;
            break;
        }
    }

    if(new_pwm_value > 255 || new_pwm_value < 0) {
        new_pwm_value = current_pwm_value;
        usart_send(status_text[1]);
    }

}