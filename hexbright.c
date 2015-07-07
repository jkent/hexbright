/* 

PWR      PB0
DRV_MODE PB1
DRV_EN   PB2 OC1B
TEMP     PC0 ADC0
CHARGE   PC3 ADC3
RLED_SW  PD2
ACC_INT  PD3
GLED     PD5
PGOOD    PD7

CHARGE Low  - charging
CHARGE HiZ  - charge error
CHARGE High - charge complete

*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <util/delay.h>
#include "util.h"
#include "hexbright.h"

#ifdef DEBUG
#define BAUD 9600
#include <stdio.h>
#include <util/setbaud.h>

static void uart_putchar(char c, FILE *stream)
{
	if (c == '\n') {
		uart_putchar('\r', stream);
	}
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
#endif

struct led_t leds[2];


void init_hardware(void)
{
	set_output(DDRB, PB0); // PWR
	set_output(DDRB, PB1); // DRV_MODE
	set_output(DDRB, PB2); // DRV_EN
	set_input(DDRD, PD2);  // RLED_SW
	set_output(DDRD, PD5); // GLED

	ICR1 = 2000;
	OCR1B = 0;
	TCCR1A |= _BV(COM1B1) | _BV(WGM11);
	TCCR1B |= _BV(WGM12) | _BV(WGM13) | _BV(CS11);

	OCR0A = 250;
	TCCR0A |= _BV(WGM01);
	TCCR0B |= _BV(CS02);
	TIMSK0 |= _BV(OCIE0A);

	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

#ifdef DEBUG
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~(_BV(U2X0));
#endif
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UCSR0B = _BV(TXEN0);
	
	stdout = &uart_stdout;
#endif
}

void sleep(void)
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	sleep_bod_disable();
	sei();
	sleep_cpu();
	sleep_disable();
	cli();
}

void update_leds(void)
{
	uint8_t i;
	led_t *led;

	for (i = 0; i < 2; i++) {
		led = &leds[i];
		
		if (led->state & 4) {
			led->count++;
			if ((led->state & 1) && (led->count >= led->on_time)) {
				led->state &= ~1;
				led->count = 0;
			} else if (!(led->state & 1) && (led->count >= led->off_time)) {
				led->state |= 1;
				led->count = 0;
			}
		} else {
			led->count = 0;
		}

		if (i == RED_LED) {
			if (led->state & 1) {
				set_output(DDRD, PD2);
				output_high(PORTD, PD2);
			} else {
				if (led->state & 2) {
					set_output(DDRD, PD2);
				} else {
					set_input(DDRD, PD2);
				}
				output_low(PORTD, PD2);
			}
		} else if (i == GREEN_LED) {
			set_output(DDRD, PD5);
			if (led->state & 1) {
				output_high(PORTD, PD5);	
			} else {
				output_low(PORTD, PD5);
			}
		}
	}
}

uint32_t poll_button(void)
{
	static uint8_t debounce = DEBOUNCE_TIME - 1;
	static uint8_t button = 0;
	
	set_output(DDRD, PD2);
	output_low(PORTD, PD2);
	_delay_us(1);
	set_input(DDRD, PD2);

	button = (button & 1) | ((button & 1) << 1);
	if (PIND & (1 << PD2)) {
		if (debounce < DEBOUNCE_TIME) {
			debounce += 1;
			if (debounce >= DEBOUNCE_TIME) {
				button |= 1;
			}
		}
	} else {
		if (debounce > 0) {
			debounce -= 1;
			if (debounce <= 1) {
				button &= ~1;
			}
		}
	}

	return button;
}

void set_light(uint16_t brightness)
{
	if (brightness <= 1000) {
		output_low(PORTB, PB1);
		OCR1B = brightness * 2;
	} else {
		output_high(PORTB, PB1);
		OCR1B = (brightness - 750) * 1.6;
	}
}

void set_power(uint8_t state)
{
	if (state) {
		output_high(PORTB, PB0);
	} else {
		output_low(PORTB, PB0);
	}
}

static uint16_t ReadADC(uint8_t channel)
{
	ADMUX = (ADMUX & 0xF0) | channel;
	ADCSRA |= _BV(ADSC);
	loop_until_bit_is_set(ADCSRA, ADIF);
	ADCSRA |= _BV(ADIF);

	return ADC;
}

charge_status_t poll_charger(void)
{
	uint16_t value;
	charge_status_t read_status;
	static charge_status_t pending_status = CHARGE_SHUTDOWN;
	static charge_status_t status = CHARGE_SHUTDOWN;
	static uint8_t count = 0;

	value = ReadADC(3);

	if (count < 255) {
		count++;
	}

	if (value >= 896) {
		read_status = CHARGE_FINISHED;
	} else if (value < 128) {
		read_status = CHARGE_CHARGING;
	} else {
		read_status = CHARGE_SHUTDOWN;
	}

	if (read_status != pending_status) {
		pending_status = read_status;
		count = 0;
	}

	if ((count >= 2 && pending_status != CHARGE_SHUTDOWN) || count >= 31) {
		status = pending_status;
	}

	return status;
}

uint16_t poll_temp(void)
{
	return ReadADC(0);
}

