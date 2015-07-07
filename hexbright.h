#ifndef HEXBRIGHT_H
#define HEXBRIGHT_H

#define DEBOUNCE_TIME 6 // 48ms

#define RED_LED 0
#define GREEN_LED 1

#define LED_HIZ   0
#define LED_OFF   2
#define LED_ON    3
#define LED_FLASH 4

typedef struct led_t led_t;
typedef enum charge_status_t charge_status_t;

struct led_t {
	uint8_t state;
	uint8_t on_time;
	uint8_t off_time;
	uint8_t count;
};

extern led_t leds[2];

enum charge_status_t {
	CHARGE_SHUTDOWN = 0,
	CHARGE_CHARGING,
	CHARGE_FINISHED,
};

void init_hardware(void);
void sleep(void);
void update_leds(void);
uint32_t poll_button(void);
void set_light(uint16_t brightness);
void set_power(uint8_t state);
charge_status_t poll_charger(void);
uint16_t poll_temp(void);

#endif /* HEXBRIGHT_H */
