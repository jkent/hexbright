#include <avr/interrupt.h>
#include "hexbright.h"

#ifdef DEBUG
#include <stdio.h>
#endif

#define OVERTEMP 340

EMPTY_INTERRUPT(TIMER0_COMPA_vect);

typedef enum mode_t mode_t;

enum mode_t {
	MODE_OFF = 0,
	MODE_CYCLE_LEVEL1,
	MODE_CYCLE_LEVEL2,
	MODE_CYCLE_LEVEL3,
	MODE_CYCLE_LEVEL4,
	MODE_CYCLE_LEVEL5,
	MODE_CYCLE_END,
	MODE_PERSIST_BLINKY,
	MODE_PERSIST_END,
	MODE_HOLD_BLINKY,
	MODE_HOLD_END,
};

int main(void)
{
	uint8_t button;
	mode_t mode = MODE_OFF;
	mode_t next_mode = MODE_OFF;
	
	uint8_t up_time = 0;
	uint8_t down_time = 0;
	uint8_t time = 0;

	init_hardware();

	leds[GREEN_LED].state = LED_OFF;
	leds[GREEN_LED].on_time = 31;
	leds[GREEN_LED].off_time = 31;

	leds[RED_LED].state = LED_HIZ;


	while (1) {
		button = poll_button();

		if (button & 2) {
			if (down_time < 255) {
				down_time++;
			}
			up_time = 0;
		} else {
			if (up_time < 255) {
				up_time++;
			}
			down_time = 0;
		}

		if (time < 255) {
			time++;
		}

		// Button pressed
		if (button == 1) {
			if (mode != MODE_OFF && up_time >= 62) {
				next_mode = MODE_OFF;
			} else if (mode < MODE_CYCLE_END) {
				next_mode = mode + 1;
				if (next_mode == MODE_CYCLE_END) {
					next_mode = MODE_OFF;
				}
			} else {
				next_mode = MODE_OFF;
			}
		}

		// Button held
		if (down_time >= 37) {
			switch (mode) {
			case MODE_CYCLE_LEVEL1:
				next_mode = MODE_HOLD_BLINKY;
				break;
			case MODE_CYCLE_LEVEL2:
				next_mode = MODE_PERSIST_BLINKY;
				break;
			default:
				break;
			}
		}

		// Button released
		if (button == 2 && mode > MODE_PERSIST_END) {
			next_mode = MODE_OFF;
		}

		if (poll_temp() >= OVERTEMP) {
			if (mode > MODE_CYCLE_LEVEL2) {
				next_mode = MODE_CYCLE_LEVEL2;
			} else if (next_mode > MODE_CYCLE_LEVEL2) {
				next_mode = MODE_OFF;
			}
		}

		if (next_mode != mode) {
			mode = next_mode;

			set_power(mode);

			switch (mode) {
			case MODE_OFF:
				set_light(0);
				break;
			case MODE_CYCLE_LEVEL1:
				set_light(50);
				break;
			case MODE_CYCLE_LEVEL2:
				set_light(250);
				break;
			case MODE_CYCLE_LEVEL3:
				set_light(1000);
				break;
			case MODE_CYCLE_LEVEL4:
				set_light(1500);
				break;
			case MODE_CYCLE_LEVEL5:
				set_light(2000);
				break;
			case MODE_PERSIST_BLINKY:
			case MODE_HOLD_BLINKY:
				time = 0;
				set_light(2000);
				break;
			default:
				mode = MODE_OFF;
				break;
			}
		}

		switch (mode) {
		case MODE_PERSIST_BLINKY:
		case MODE_HOLD_BLINKY:
			if (time >= 24) {
				set_light(2000);
				time = 0;
			} else if (time >= 8) {
				set_light(0);
			}
			break;
		default:
			break;
		}

		switch (poll_charger()) {
		case CHARGE_SHUTDOWN:
			leds[GREEN_LED].state = LED_OFF;
			break;
		case CHARGE_CHARGING:
			leds[GREEN_LED].state |= LED_FLASH;
			break;
		case CHARGE_FINISHED:
			leds[GREEN_LED].state = LED_ON;
			break;
		}

		update_leds();
		sleep();
	}

	return 0;
}
